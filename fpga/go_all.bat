@echo off

rmdir/s/q xst

del fpga_all.ngc
xst -ifn xst_all.scr
if errorlevel 0 goto ok1
goto done
:ok1

del fpga_all.ngd
ngdbuild -aul -p xc3s250e-4-vq100 -nt timestamp -uc fpga.ucf fpga_all.ngc fpga_all.ngd
if errorlevel 0 goto ok2
goto done
:ok2

del fpga_all.ncd
map -p xc3s250e-4-vq100 fpga_all.ngd
if errorlevel 0 goto ok3
goto done
:ok3

del fpga_all-placed.ncd
par fpga_all.ncd fpga_all-placed.ncd
if errorlevel 0 goto ok4
goto done
:ok4

del fpga_all.bit fpga_all.drc fpga_all.rbt
bitgen -b fpga_all-placed.ncd fpga_all.bit
if errorlevel 0 goto ok5
goto done
:ok5


echo okay
REM perl ..\tools\rbt2c.pl fpga_lf.rbt > ..\armsrc\fpgaimg.c

:done
