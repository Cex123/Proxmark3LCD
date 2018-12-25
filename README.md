# proxmark3-lcd: port of official Proxmark repository to support pm3-lcd v1.17 by d18c7db

This repository contains a port of the v3.1.0 version of Proxmark firmware to be used in pm3-lcd v1.17 by d18c7db.

## Resources

* [Proxmark repository!](https://github.com/Proxmark/proxmark3)
* [The Forum](http://www.proxmark.org/forum/viewtopic.php?id=436)
* [Null Space Labs](http://wiki.032.la/proxmark3_lcd) (no longer available)
* My fisrt port on Google Code: http://proxmark3.googlecode.com/svn/trunk (no longer available)
   
## Development

The tools required to build  or run the project are the same as for Proxmark3. Please refer 
to [the wiki](https://github.com/Proxmark/proxmark3/wiki) for details.

## Obtaining hardware

The Proxmark3-lcd PCB was available from BatchPCB, but no longer is.
If you want to build one of these you'll need to order the PCBs by yourself.
Note that the HF path is not working well and a few changes are required (thanks to Rotte for these):
 - Change R405 value from 10K to 100K
 - Change C414 value from 1nF to something between 200pF and 400pF
 - Change R412 value from 100K to something between 1K and 2.2K (NOTE that this will cause LF path failure, 
 so better remove R412 and put two resistors before the analog MUX (CD4066) one of value 100K for LF path, and
 another one of 1K for HF path).


##TODO
 - Add a GLCD menu instead of hardcoding menu items. Maybe using https://exploreembedded.com/wiki/Interactive_Menus_for_your_project_with_a_Display_and_an_Encoder
 - Add more options to current menus.
 - Document HW changes to HF path.

## License

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Original Proxmark3 developed by:

Jonathan Westhues
user jwesthues, at host cq.cx

May 2007, Cambridge MA
