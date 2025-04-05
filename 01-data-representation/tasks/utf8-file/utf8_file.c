#include <sys/types.h>
#include <unistd.h>

#include "utf8_file.h"

#define ERROR -1

const uint32_t ONE_BYTE = 7;
const uint32_t TWO_BYTE = 11;
const uint32_t TREE_BYTE = 16;
const uint32_t FOUR_BYTE = 21;
const uint32_t FIVE_BYTE = 26;
const uint32_t SIX_BYTE = 31;
const uint32_t USEFULL_BITS_NUMBERS_IN_LOWER_BITES = 6;
const uint32_t BYTE_LENGHT = 8;
const uint8_t META_LOWER_BYTE = 0b10000000;

size_t len_str(uint64_t number) {
  size_t ans = 0;
  while (number != 0) {
    number = number >> 1;
    ++ans;
  }
  return ans;
}

uint64_t get_bit_mask(size_t begin, size_t end) {
  uint64_t num1 = (1 << (begin)) - 1;
  uint64_t num2 = (1 << (end)) - 1;
  return num2 - num1;
}

uint8_t get_bit_mask_in_num(uint32_t symbol, size_t begin, size_t end) {
  return (uint8_t)((get_bit_mask(begin, end) & symbol) >> begin);
}

int lower_byte_count(uint64_t len_symbol) {
  if (len_symbol <= ONE_BYTE) {
    return 0;
  }
  if (len_symbol <= TWO_BYTE) {
    return 1;
  }
  if (len_symbol <= TREE_BYTE) {
    return 2;
  }
  if (len_symbol <= FOUR_BYTE) {
    return 3;
  }
  if (len_symbol <= FIVE_BYTE) {
    return 4;
  }
  return ((len_symbol <= SIX_BYTE) ? 5 : ERROR);
}

int write_high_byte(utf8_file_t* f, uint32_t symbol) {
  size_t len_symbol = len_str(symbol);
  size_t byte_numbers = lower_byte_count(len_symbol);

  uint8_t meta =
      (uint8_t)get_bit_mask(BYTE_LENGHT - byte_numbers - 1, BYTE_LENGHT);
  if (byte_numbers == 0) {
    meta = 0;
  }
  size_t usefull_bits_count =
      len_symbol - USEFULL_BITS_NUMBERS_IN_LOWER_BITES * byte_numbers;

  uint8_t high_byte =
      (uint8_t)((get_bit_mask(len_symbol - usefull_bits_count, len_symbol) &
                 symbol) >>
                (len_symbol - usefull_bits_count)) +
      meta;

  if (write(f->fd, &high_byte, 1) < 0) {
    return ERROR;
  }
  return 0;
}

int utf8_write(utf8_file_t* f, const uint32_t* str, size_t count) {
  size_t i = 0;
  for (; i < count; ++i) {
    size_t len_symbol = len_str(str[i]);
    size_t lower_byte_numbers = lower_byte_count(len_symbol);
    if (write_high_byte(f, str[i]) != 0) {
      printf("utf8_write: can't write lead byte\n");
      return ERROR;
    }
    if (lower_byte_numbers == 0) {
      continue;
    }

    uint8_t meta = (uint8_t)get_bit_mask(
        USEFULL_BITS_NUMBERS_IN_LOWER_BITES + 1, BYTE_LENGHT);
    uint8_t lower_byte = meta;
    size_t shift = USEFULL_BITS_NUMBERS_IN_LOWER_BITES * lower_byte_numbers;

    for (size_t j = 0; j < lower_byte_numbers; ++j) {
      lower_byte += get_bit_mask_in_num(
          str[i], shift - USEFULL_BITS_NUMBERS_IN_LOWER_BITES, shift);
      shift -= USEFULL_BITS_NUMBERS_IN_LOWER_BITES;
      if (write(f->fd, &lower_byte, 1) < 0) {
        printf("utf8_write: can't write lower byte\n");
        return ERROR;
      }
    }
  }
  return i;
}

int leading_units_number(uint8_t high_byte) {
  int i = 0;
  while (get_bit_mask_in_num(high_byte, (BYTE_LENGHT - 1) - i, BYTE_LENGHT) ==
         (uint8_t)get_bit_mask(0, i + 1)) {
    ++i;
  }
  if (i == 0) return 0;
  return i - 1;
}

int utf8_read(utf8_file_t* f, uint32_t* res, size_t count) {
  size_t i = 0;
  for (; i < count; ++i) {
    res[i] = 0;
    uint8_t high_byte = 0;
    int cnt = read(f->fd, &high_byte, 1);
    if (cnt < 0) {
      printf("utf8_read: can't read lead byte\n");
      return ERROR;
    } else if (cnt == 0 && i == 0) {
      return 0;
    } else if (cnt == 0) {
      return i;
    }

    int lower_bytes_number = leading_units_number(high_byte);
    if (lower_bytes_number == 0) {
      res[i] = high_byte;
      continue;
    } else if (lower_bytes_number > 5) {
      printf("utf8_read: it's not utf8\n");
      return ERROR;
    }
    high_byte -=
        get_bit_mask(BYTE_LENGHT - lower_bytes_number - 1, BYTE_LENGHT);
    res[i] +=
        high_byte << (lower_bytes_number * USEFULL_BITS_NUMBERS_IN_LOWER_BITES);

    for (size_t j = 0; j < lower_bytes_number; ++j) {
      uint8_t lower_byte = 0;
      int cnt1 = read(f->fd, &lower_byte, 1);
      if (cnt1 < 0) {
        printf("utf8_read: can't read lower byte\n");
        return ERROR;
      } else if (cnt == 0) {
        return i;
      }
      lower_byte -= META_LOWER_BYTE;
      res[i] += lower_byte << (USEFULL_BITS_NUMBERS_IN_LOWER_BITES *
                               (lower_bytes_number - 1 - j));
    }
  }
  return i;
}

utf8_file_t* utf8_fromfd(int fd) {
  utf8_file_t* file = (utf8_file_t*)malloc(sizeof(utf8_file_t));
  file->fd = fd;
  return file;
}
