#include "uart_cmd.h"
#include "wheel.h"

#define BAUD_RATE 115200UL
#define RX_BUF_SIZE 128U /* power of two */
#define LINE_MAX_LEN 64U

static volatile uint8_t rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head;
static volatile uint16_t rx_tail;

static char line[LINE_MAX_LEN];
static uint8_t line_len;

/* ---------- UART low level ---------- */

void UartCmd_Init(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();

  gpio.Pin = GPIO_PIN_9; /* TX */
  gpio.Mode = GPIO_MODE_AF_PP;
  gpio.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &gpio);

  gpio.Pin = GPIO_PIN_10; /* RX */
  gpio.Mode = GPIO_MODE_INPUT;
  gpio.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &gpio);

  /* USART1 is on APB2 = 72 MHz */
  USART1->BRR = (HAL_RCC_GetPCLK2Freq() + BAUD_RATE / 2U) / BAUD_RATE;
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE;

  HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void USART1_IRQHandler(void)
{
  uint32_t sr = USART1->SR;
  if (sr & (USART_SR_RXNE | USART_SR_ORE))
  {
    uint8_t byte = (uint8_t)USART1->DR; /* reading DR clears RXNE/ORE */
    uint16_t next = (uint16_t)((rx_head + 1U) & (RX_BUF_SIZE - 1U));
    if (next != rx_tail)
    {
      rx_buf[rx_head] = byte;
      rx_head = next;
    }
  }
}

static void uart_putc(char c)
{
  while (!(USART1->SR & USART_SR_TXE))
  {
  }
  USART1->DR = (uint8_t)c;
}

static void uart_puts(const char *s)
{
  while (*s)
  {
    uart_putc(*s++);
  }
}

static void uart_put_uint(unsigned v)
{
  char tmp[10];
  int n = 0;
  do
  {
    tmp[n++] = (char)('0' + v % 10U);
    v /= 10U;
  } while (v);
  while (n)
  {
    uart_putc(tmp[--n]);
  }
}

/* ---------- command parsing ---------- */

/* Parse a decimal number like 12, -3, 0.8, .5 (no exponent). Returns 1 on success. */
static int parse_number(const char **pp, float *out)
{
  const char *p = *pp;
  int neg = 0;
  int digits = 0;
  float value = 0.0f;

  if (*p == '-')
  {
    neg = 1;
    p++;
  }
  else if (*p == '+')
  {
    p++;
  }
  while (*p >= '0' && *p <= '9')
  {
    value = value * 10.0f + (float)(*p - '0');
    digits++;
    p++;
  }
  if (*p == '.')
  {
    float scale = 0.1f;
    p++;
    while (*p >= '0' && *p <= '9')
    {
      value += scale * (float)(*p - '0');
      scale *= 0.1f;
      digits++;
      p++;
    }
  }
  if (!digits)
  {
    return 0;
  }
  *out = neg ? -value : value;
  *pp = p;
  return 1;
}

static int starts_with(const char *s, const char *prefix)
{
  while (*prefix)
  {
    char a = *s++;
    if (a >= 'A' && a <= 'Z')
    {
      a = (char)(a + 32);
    }
    if (a != *prefix++)
    {
      return 0;
    }
  }
  return 1;
}

static void reply_error(const char *msg)
{
  uart_puts("ERR ");
  uart_puts(msg);
  uart_puts("\r\n");
}

static void handle_set_wheel(const char *p)
{
  float arg[3];

  p += 9; /* strlen("set_wheel") */
  for (int i = 0; i < 3; i++)
  {
    /* skip separators: spaces, '(' ',' ';' */
    while (*p == ' ' || *p == '(' || *p == ',' || *p == ';' || *p == '\t')
    {
      p++;
    }
    if (!parse_number(&p, &arg[i]))
    {
      reply_error("usage: set_wheel(wheel 0..3, duty 0..1, direction 0|1)");
      return;
    }
  }
  while (*p == ' ' || *p == ')' || *p == ';' || *p == '\t')
  {
    p++;
  }
  if (*p != '\0')
  {
    reply_error("usage: set_wheel(wheel 0..3, duty 0..1, direction 0|1)");
    return;
  }

  if (arg[0] != (float)(int)arg[0] || arg[0] < 0.0f || arg[0] > 255.0f)
  {
    reply_error("wheel must be 0, 1, 2 or 3");
    return;
  }
  if (arg[2] != (float)(int)arg[2] || arg[2] < 0.0f || arg[2] > 255.0f)
  {
    reply_error("direction must be 0 (forward) or 1 (backward)");
    return;
  }

  uint8_t wheel = (uint8_t)arg[0];
  uint8_t dir = (uint8_t)arg[2];
  int rc = set_wheel(wheel, arg[1], dir);

  switch (rc)
  {
  case WHEEL_OK:
    uart_puts("OK wheel=");
    uart_put_uint(wheel);
    uart_puts(" duty=");
    uart_put_uint((unsigned)(arg[1] * 100.0f + 0.5f));
    uart_puts("% dir=");
    uart_putc(dir ? 'B' : 'F');
    uart_puts("\r\n");
    break;
  case WHEEL_ERR_NUMBER:
    reply_error("wheel must be 0, 1, 2 or 3");
    break;
  case WHEEL_ERR_DUTY:
    reply_error("duty must be between 0 and 1");
    break;
  default:
    reply_error("direction must be 0 (forward) or 1 (backward)");
    break;
  }
}

static void handle_line(const char *s)
{
  while (*s == ' ' || *s == '\t')
  {
    s++;
  }
  if (*s == '\0')
  {
    return;
  }
  if (starts_with(s, "set_wheel"))
  {
    handle_set_wheel(s);
  }
  else if (starts_with(s, "stop"))
  {
    Wheel_StopAll();
    uart_puts("OK stopped\r\n");
  }
  else
  {
    reply_error("unknown command. Use: set_wheel(0, 0.8, 0) or stop");
  }
}

void UartCmd_Poll(void)
{
  while (rx_tail != rx_head)
  {
    char c = (char)rx_buf[rx_tail];
    rx_tail = (uint16_t)((rx_tail + 1U) & (RX_BUF_SIZE - 1U));

    if (c == '\r' || c == '\n')
    {
      uart_puts("\r\n");
      line[line_len] = '\0';
      line_len = 0;
      handle_line(line);
    }
    else if (c == 0x08 || c == 0x7F) /* backspace */
    {
      if (line_len)
      {
        line_len--;
        uart_puts("\b \b");
      }
    }
    else if (line_len < LINE_MAX_LEN - 1U)
    {
      line[line_len++] = c;
      uart_putc(c); /* echo for plain serial terminals */
    }
  }
}
