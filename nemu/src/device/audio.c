/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <common.h>
#include <device/map.h>
#include <SDL2/SDL.h>

enum {
  reg_freq,
  reg_channels,
  reg_samples,
  reg_sbuf_size,
  reg_init,
  reg_count,
  nr_reg
};

static uint8_t *sbuf = NULL;
static uint8_t audio_buffer[CONFIG_SB_SIZE];
static uint32_t buf_count = 0;
static uint32_t buf_head = 0;
static uint32_t buf_tail = 0;
static uint32_t *audio_base = NULL;
static size_t sbuf_count = 0;

static inline void set_buf_count(uint32_t c) {
  buf_count = c;
  audio_base[reg_count] = buf_count;
}

static void audio_callback(void *userdata, uint8_t *stream, int len) {
  int i = 0;
  for (; i < len && i < buf_count; i++) {
    stream[i] = audio_buffer[buf_head];
    buf_head = (buf_head + 1) % CONFIG_SB_SIZE;
  }
  set_buf_count(buf_count - i);
  sbuf_count += i;
  for (; i < len; i++) {
    stream[i] = 0;
  }
}

static SDL_AudioSpec s = {};
static void audio_io_handler(uint32_t offset, int len, bool is_write) {
  int reg = offset / 4;
  assert(reg < nr_reg);
  if (is_write) {
    assert(reg != reg_sbuf_size);
    if (reg == reg_init) {
      s.format = AUDIO_S16SYS;  // 假设系统中音频数据的格式总是使用16位有符号数来表示
      s.userdata = NULL;        // 不使用
      s.freq = *(audio_base + reg_freq);
      s.channels = *(audio_base + reg_channels);
      s.samples = *(audio_base + reg_samples);
      s.callback = audio_callback;
      SDL_InitSubSystem(SDL_INIT_AUDIO);
      SDL_OpenAudio(&s, NULL);
      SDL_PauseAudio(0);
    }
  }
}

static void audio_sbuf_handler(uint32_t offset, int len, bool is_write) {
  assert(is_write == 1);
  assert(CONFIG_SB_SIZE - buf_count >= len);
  SDL_LockAudio();
  for (int i = 0; i < len; i++) {
    audio_buffer[buf_tail] = sbuf[offset+i];
    buf_tail = (buf_tail + 1) % CONFIG_SB_SIZE;
  }
  set_buf_count(buf_count + len);
  SDL_UnlockAudio();
}

void init_audio() {
  uint32_t space_size = sizeof(uint32_t) * nr_reg;
  audio_base = (uint32_t *)new_space(space_size);
  *(audio_base + reg_sbuf_size) = CONFIG_SB_SIZE;
#ifdef CONFIG_HAS_PORT_IO
  add_pio_map ("audio", CONFIG_AUDIO_CTL_PORT, audio_base, space_size, audio_io_handler);
#else
  add_mmio_map("audio", CONFIG_AUDIO_CTL_MMIO, audio_base, space_size, audio_io_handler);
#endif

  sbuf = (uint8_t *)new_space(CONFIG_SB_SIZE);
  set_buf_count(0);
  add_mmio_map("audio-sbuf", CONFIG_SB_ADDR, sbuf, CONFIG_SB_SIZE, audio_sbuf_handler);
}
