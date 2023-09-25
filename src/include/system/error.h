/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/*                  Copyright (c) (2021 - 2023) Nicolaus Starke               */
/*                  https://github.com/nic-starke/muffintwister               */
/*                   SPDX-License-Identifier: GPL-3.0-or-later                */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Includes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#define EXIT_ON_ERR(s, l)                                                      \
  do {                                                                         \
    if (s != SUCCESS) {                                                        \
      goto l;                                                                  \
    }                                                                          \
  } while (0)

#define RETURN_ON_ERR(s)                                                       \
  do {                                                                         \
    if (s != SUCCESS) {                                                        \
      return s;                                                                \
    }                                                                          \
  } while (0)

#define EXIT_IF_NULL(p, l)                                                     \
  do {                                                                         \
    if (p == NULL) {                                                           \
      goto l;                                                                  \
    }                                                                          \
  } while (0)

#define RETURN_ERR_IF_NULL(p)                                                  \
  do {                                                                         \
    if (p == NULL) {                                                           \
      return ERR_NULL_PTR;                                                     \
    }                                                                          \
  } while (0)

#define RETURN_IF_NULL(p)                                                      \
  do {                                                                         \
    if (p == NULL) {                                                           \
      return;                                                                  \
    }                                                                          \
  } while (0)

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Extern ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

enum {
  SUCCESS       = 0,
  ERR_BAD_PARAM = -1,
  ERR_NULL_PTR  = -2,
};

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Variables ~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Global Functions ~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Local Functions ~~~~~~~~~~~~~~~~~~~~~~~~~ */
