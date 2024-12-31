# LoggerAutoProg
## Описание
Утилита предназначена для автоматизации процесса серийной прошивки устройств

## Инструкция по использованию
Для использования утилиты `LoggerAutoProg` необходимо наличие программатора `GD-Link`
### Подготовка среды
- Загрузить и установить [Microsoft Visual C++ Redistributable](../../releases/download/v1.0/VC_redist.exe)
- Загрузить архив, содержащий последнюю версию утилиты из [releases](../../releases)
- Извлечь все файлы из архива в одну пустую директорию. Получившийся путь не должен содержать символов кириллицы
- Загрузить последнюю версию `LoggerSoft.bin`
- Поместить загруженный `LoggerSoft.bin` в одну директорию с распакованными файлами
- Запустить `LoggerAutoProg.exe`
- Нажать `Файл`
- В выпадающем меню нажать `Открыть...`
- В открывшемся окне найти и выбрать файл `LoggerSoft.bin`, предварительно помещенный в одну директорию с `LoggerAutoProg.exe` на одном из предыдущих шагов
- Вручную задать `id` первого прошиваемого устройства, для этого:
  - Снять флажок `Safety`
  - Ввести желаемый целочисленный положительный `id` в текстовое поле
  - Подтвердить ввод нажатием кнопки `Ввести id`
  - Убедиться в правильности своих действий: в строке `Текущий id` должно отобразиться введенное число
  - Поставить флажок `Safety`
- Приступить к серийной прошивке устройств
### Серийная прошивка устройств
- Подключить устройство к компьютеру при помощи программатора
- Нажать на кнопку `Подключить`, дождаться успешного выполнения подключения
- Нажать на кнопку `Прошить`, дождаться успешной загрузки прошивки во внутренню память микроконтроллера
- Если всё прошло успешно, `Текущий id` будет автоматически увеличен на 1, а в строке состояния появится сообщение о том, что устройство с предыдущим `id` успешно прошито
- Отключить устройство от компьютера и подключить следующее устройство для выполнения аналогичных действий

## Прочая справочная информация для разработчиков
### Информация по `GD_Link_CLI`

#### Ответ на `?` от `GD_Link_CLI.exe`
```
Available commands are:
-------------------------------------------------------
mem        Read memory. Syntax: mem  [<Zone>:]<Addr>, <NumBytes> (hex)
mem8       Read  8-bit items. Syntax: mem8  [<Zone>:]<Addr>, <NumBytes> (hex)
mem16      Read 16-bit items. Syntax: mem16 [<Zone>:]<Addr>, <NumItems> (hex)
mem32      Read 32-bit items. Syntax: mem32 [<Zone>:]<Addr>, <NumItems> (hex)
w1         Write  8-bit items. Syntax: w1 [<Zone>:]<Addr>, <Data> (hex)
w2         Write 16-bit items. Syntax: w2 [<Zone>:]<Addr>, <Data> (hex)
w4         Write 32-bit items. Syntax: w4 [<Zone>:]<Addr>, <Data> (hex)
erase      Erase internal flash of selected device. Syntax: Erase
r          Reset target         (RESET)
g          go
h          halt
step       step
loadbin    Load *.bin file into target memory.
           Syntax: loadbin <filename>, <Addr>
savebin    Saves target memory into binary file.
           Syntax: savebin <filename>, <Addr>, <NumBytes>
setOPT     Load *.bin file into option bytes.
           Syntax: setopt <filename>
readOPT    Saves option bytes into binary file.
           Syntax: readopt <filename>, <NumBytes>
writeOTP   Load *.bin file into OTP block.
           Syntax: writeotp <filename>, <Addr>
readOTP    Saves OTP block into binary file.
           Syntax: readotp <filename>, <Addr>, <NumBytes>
SetPC      Set the PC to specified value. Syntax: SetPC <Addr>
SetRDP     Set the RDP(Read Protect) to specified level. Syntax: SetRDP <level>
ReadAP     Read a CoreSight AP register. Syntax: ReadAP <Addr>
WriteAP    Write a CoreSight AP register. Syntax: WriteAP <Addr>, <Data> (hex)
ReadDP     Reads a CoreSight DP register. Syntax: ReadDP <Addr>
WriteDP    Write a CoreSight DP register. Syntax: WriteDP <Addr>, <Data> (hex)
si         Change target interface. Syntax: si <interface>
           where 0=JTAG and 1=SWD
sd         Set up the device Part No. manually. Syntax: sd <Part No.>
SelectJTAGDevice         Select the JTAG device Index. manually. Syntax: SelectJTAGDevice <JTAG device Index>
WriteIR         Write IR register in JTAG port. manually. Syntax: WriteIR <IR Register>(hex)
WriteReadDR         Write and read DR Data in JTAG port. manually. Syntax: WriteReadDR <TDIData>(hex), <TDIDataBitLength>(decimal)
c         Change the USB device. manually. Syntax: c <SN>
q          Quit
-------------------------------------------------------
```

#### Команды, найденные в `GD_Link_CLI.exe` через hex-редактор
```
connect
mem
mem8
mem16
mem32
w1
w2
w4
erase
cmp
reset
r
g
go
h
halt
step
loadbin
savebin
setpc
setrdp
readap
writeap
readdp
writedp
si
sleep
q
selectjtagdevice
writeir
writereaddr
c
readopt
setopt
readotp
writeotp
```
