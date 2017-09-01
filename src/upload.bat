@ECHO OFF &SETLOCAL

set comport=COM3
set files=
for /r %%a in (firmware\*.bin) do call set files=%%files%% %%~na firmware\%%~na.bin

if "%files%" == "" echo No file to upload & exit /b

echo Writing %files% to %comport%

REM esptool.py -p %comport% --flash_size 32m write_flash %files%
esptool.py --port %comport% write_flash --flash_mode dio --flash_size detect %files%

if NOT "%errorlevel%" == "0" echo Upload Failed & exit /b

miniterm.py %comport% 74880

