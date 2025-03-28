#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/rand.h>

#define TRIVIUM_STATE_SIZE 288
#define OUTPUT_BIT_SIZE 10000000

struct TriviumState
{
  uint8_t state[TRIVIUM_STATE_SIZE];
};

// RANDOM BYTE GENERATOR

void generate_random_bytes(uint8_t *buffer, size_t length)
{
  arc4random_buf(buffer, length);
}

// GENERATE TRIVIUM KEYSTREAM
uint8_t generate_trivium_keystream(struct TriviumState *trivium, int shifts)
{
  uint8_t keystream = 0;

  for (int i = 0; i < shifts; i++)
  {
    uint8_t t1 = trivium->state[65] ^ trivium->state[92];
    uint8_t t2 = trivium->state[161] ^ trivium->state[177];
    uint8_t t3 = trivium->state[242] ^ trivium->state[287];

    uint8_t output_bit = t1 ^ t2 ^ t3;

    t1 = t1 ^ (trivium->state[90] & trivium->state[91]) ^ trivium->state[170];
    t2 = t2 ^ (trivium->state[174] & trivium->state[175]) ^ trivium->state[263];
    t3 = t3 ^ (trivium->state[285] & trivium->state[286]) ^ trivium->state[68];

    // right shift
    memmove(trivium->state + 1, trivium->state, TRIVIUM_STATE_SIZE - 1);

    trivium->state[0] = t3;
    trivium->state[93] = t1;
    trivium->state[177] = t2;

    keystream = (uint8_t)((keystream << (uint8_t)1) | (uint8_t)output_bit);
  }

  return keystream;
}

// INITIALIZE TRIVIUM (places key and initialization vector)
void initialize_trivium(struct TriviumState *trivium, uint8_t *key, uint8_t *iv)
{
  memset(trivium->state, 0, TRIVIUM_STATE_SIZE); // initialize all values to 0

  // load 80 bit key into position (0-79)
  for (int i = 0; i < 80; i++)
  {
    trivium->state[i] = (key[i / 8] >> (i % 8)) & 1;
  }

  // load 80 bit iv into position (93-..)
  for (int i = 93; i < 173; i++)
  {
    trivium->state[i] = (iv[(i - 93) / 8] >> ((i - 93) % 8)) & 1;
  }

  // last 3 are 1s
  trivium->state[285] = 1;
  trivium->state[286] = 1;
  trivium->state[287] = 1;

  for (int i = 0; i < 1152; i++)
  {
    generate_trivium_keystream(trivium, 1);
  }
}

void generator(FILE *f, uint8_t *key, uint8_t *iv, int bits)
{
  struct TriviumState *trivium = (struct TriviumState *)malloc(sizeof(struct TriviumState));
  initialize_trivium(trivium, key, iv);

  for (int i = 0; i < bits / 8; i++)
  {
    uint8_t byte = generate_trivium_keystream(trivium, 8);
    fwrite(&byte, 1, 1, f);
  }

  fclose(f);
}

int main(void)
{
  uint8_t key[10];
  uint8_t iv[10];

  generate_random_bytes(key, sizeof(key));
  generate_random_bytes(iv, sizeof(iv));

  FILE *f = fopen("sts-2.1.2/trivium_output.bin", "wb");
  generator(f, key, iv, OUTPUT_BIT_SIZE);

  printf("Saved keystream");

  return 1;
}
