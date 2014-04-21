/**
 * @file tal_pib.h
 *
 * @brief This file contains the prototypes for TAL PIB functions.
 *
 * $Id: tal_pib.h 16323 2009-06-23 16:38:09Z sschneid $
 *
 * @author    Atmel Corporation: http://www.atmel.com
 * @author    Support email: avr@atmel.com
 */
/*
 * Copyright (c) 2009, Atmel Corporation All rights reserved.
 *
 * Licensed under Atmel's Limited License Agreement --> EULA.txt
 */

/* Prevent double inclusion */
#ifndef TAL_PIB_H
#define TAL_PIB_H

/* === INCLUDES ============================================================ */


/* === EXTERNALS =========================================================== */


/* === TYPES =============================================================== */


/* === MACROS ============================================================== */

#define RF_TX_POWER_LEVEL_MAX	21 // There are 22 entries in the table that range from 0 to 21

/* === PROTOTYPES ========================================================== */

#ifdef __cplusplus
extern "C" {
#endif


void init_tal_pib(void);
void write_all_tal_pib_to_trx(void);


#ifdef __cplusplus
} /* extern "C" */
#endif


#endif /* TAL_PIB_H */

/* EOF */
