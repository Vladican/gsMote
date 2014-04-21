ATMEL Corporation Inc.

This file contains important information for the MAC used for
802.15.4 transceivers and microcontrollers of ATMEL Corp.

For license information see file:
EULA.txt.

For
- Release and version information of the MAC
- System requirements - Supported Compiler/Tools
see file:
Doc\MAC_Release_Notes.txt.

For the AVR2025 IEEE 802.15.4 MAC Software Package User Guide see file:
Doc\User_Guide\AVR2025_User_Guide.pdf.

For the AVR2025 IEEE 802.15.4 MAC Reference Manual including the API see file:
MAC_readme.html.



Installation
- Unzip release file to a top level folder, like "c:\Atmel".

- Supporting the ATmega128RFA1 for IAR Embedded Workbench (Version 5.40.1)
  requires replacing the existing register file "iom128rfa1.h"
  that is installed with the IAR Workbench (located in directory "\avr\inc"
  of the IAR Workbench installation directory) by the register file
  (with the same name) that is provided by IAR systems as patch file.

- The Windows USB Driver (.inf) for the RZUSBSTICK installation can be found at
  "PAL\AVR\AT90USB1287\Boards\USBSTICK_C\rzusbstick_cdc.inf"


Applications
The included example applications can be built by using the corresponding
makefiles for GCC or the corresponding IAR project files
(e.g. Coordinator.eww).

For further information about the build process please refer to the
AVR2025 IEEE 802.15.4 MAC Software Package User Guide.

$Id: IMPORTANT_README_FIRST.txt 20141 2010-02-01 16:56:11Z sschneid $