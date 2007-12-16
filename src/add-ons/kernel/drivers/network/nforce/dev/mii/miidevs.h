/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _FBSD_MII_DEVS_H
#define _FBSD_MII_DEVS_H

#define MII_OUI_CICADA				0x0003f1
#define MII_OUI_VITESSE				0x0001c1

#define MII_MODEL_CICADA_CS8201		0x0001
#define MII_MODEL_CICADA_CS8201A	0x0020
#define MII_MODEL_CICADA_CS8201B	0x0021
#define MII_MODEL_VITESSE_VSC8601	0x0002

#define MII_STR_CICADA_CS8201		"Cicada CS8201 10/100/1000TX PHY"
#define MII_STR_CICADA_CS8201A		MII_STR_CICADA_CS8201
#define MII_STR_CICADA_CS8201B		MII_STR_CICADA_CS8201
#define MII_STR_VITESSE_VSC8601		"Vitesse VSC8601 10/100/1000TX PHY"

#endif	/* _FBSD_MII_DEVS_H */
