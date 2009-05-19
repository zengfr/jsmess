#include "driver.h"
#include "cpu/z80/z80.h"

/* Read/Write Handlers */

#ifdef UNUSED_CODE
static WRITE8_HANDLER( mmr_w )
{
	/*

		Memory Mapping Register

		RAM USER MODE (0-ffff=RAM)
		b6b5b4b3b2b1b0
		0 1 0 1 0 x x

		ROM USER MODE (0-7ffff=ROM, 8000-ffff=RAM)

		0 1 0 0 0 0 0		AP is in ROM #0
		0 1 0 0 0 0 1		AP is in ROM #1
		0 1 0 0 0 1 0		AP is in ROM #2
		0 1 0 0 0 1 1		ROM cartridge AP

		CCP MODE (MENU, 0-7ffff=ROM, 8000-ffff=RAM)

		0 1 0 0 0 0 0

		KERNEL MODE (0-7ffff=ROM, 8000-ffff=RAM)

		0 0 0 0 0 0 0		S0 access
		0 0 1 0 0 0 0		S1 access
		0 1 1 0 0 0 0		RAM cartridge
		1 x x 0 0 0 0		CRT access

		etc...

	*/
}
#endif

/* Memory Maps */

static ADDRESS_MAP_START( pc8401a_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0xffff) AM_ROM
//	AM_RANGE(0x0000, 0x7fff) AM_RAMBANK(1)
//	AM_RANGE(0x8000, 0xffff) AM_RAMBANK(2)
ADDRESS_MAP_END

static ADDRESS_MAP_START( pc8401a_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( pc8401a )
INPUT_PORTS_END

/* Machine Initialization */

static MACHINE_START( pc8401a )
{
}

/* Machine Drivers */

static MACHINE_DRIVER_START( pc8401a )
//	MDRV_DRIVER_DATA(pc8401a_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(pc8401a_mem)
	MDRV_CPU_IO_MAP(pc8401a_io)
	
	MDRV_MACHINE_START( pc8401a )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 128)
	MDRV_SCREEN_VISIBLE_AREA(0, 480-1, 0, 128-1)
//	MDRV_DEFAULT_LAYOUT(layout_pc8401a)
	MDRV_PALETTE_LENGTH(2)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pc8500 )
//	MDRV_DRIVER_DATA(pc8401a_state)

	/* basic machine hardware */
	MDRV_CPU_ADD("maincpu", Z80, 4000000)
	MDRV_CPU_PROGRAM_MAP(pc8401a_mem)
	MDRV_CPU_IO_MAP(pc8401a_io)
	
	MDRV_MACHINE_START( pc8401a )

	/* video hardware */
	MDRV_SCREEN_ADD("screen", LCD)
	MDRV_SCREEN_REFRESH_RATE(44)
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(480, 200)
	MDRV_SCREEN_VISIBLE_AREA(0, 480-1, 0, 200-1)
//	MDRV_DEFAULT_LAYOUT(layout_pc8500)
	MDRV_PALETTE_LENGTH(2)
MACHINE_DRIVER_END

/* ROMs */

ROM_START( pc8401a )
	ROM_REGION( 0x18000, "maincpu", 0 )
	ROM_LOAD( "pc8401a.bin", 0x0000, 0x18000, NO_DUMP )
ROM_END

ROM_START( pc8500 )
	ROM_REGION( 0x10000, "maincpu", 0 )
	ROM_LOAD( "pc8500.bin", 0x0000, 0x10000, CRC(c2749ef0) SHA1(f766afce9fda9ec84ed5b39ebec334806798afb3) )
ROM_END

/* System Configurations */

static SYSTEM_CONFIG_START( pc8401a )
	CONFIG_RAM_DEFAULT	(64 * 1024)
SYSTEM_CONFIG_END

static SYSTEM_CONFIG_START( pc8500 )
	CONFIG_RAM_DEFAULT	(64 * 1024)
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR  NAME		PARENT	COMPAT	MACHINE		INPUT		INIT	CONFIG		COMPANY	FULLNAME */
COMP( 1984,	pc8401a,	0,		0,		pc8401a,	pc8401a,	0,		pc8401a,	"NEC",	"PC-8401A-LS", GAME_NOT_WORKING )
COMP( 1983, pc8500,		0,		0,		pc8500,		pc8401a,	0,		pc8500,		"NEC",	"PC-8500", GAME_NOT_WORKING )
