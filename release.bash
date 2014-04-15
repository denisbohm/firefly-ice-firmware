#!/bin/bash

eval `grep '^#define VERSION_' src/fd_control.c | sed -e 's/#define VERSION_\([MP][^ ]*\) \(.*\)/\1=\2/' | tr '[:upper:]' '[:lower:]'`
echo releasing version $major.$minor.$patch

cp "THUMB Flash Release/FireflyIce/FireflyIce.hex" ../firefly-ice-api/FireflyDevice/FireflyDevice
echo "#! {\"major\":$major, \"minor\":$minor, \"patch\":$patch}" >> ../firefly-ice-api/FireflyDevice/FireflyDevice/FireflyIce.hex

cp "THUMB Flash Release/FireflyBoot/FireflyBoot.elf" ../firefly-production-tools/FireflyTool/FireflyTool
cp "THUMB Flash Release/FireflyIce/FireflyIce.elf" ../firefly-production-tools/FireflyTool/FireflyTool
cp "THUMB RAM Debug/FireflyRadioTest/FireflyRadioTest.elf" ../firefly-production-tools/FireflyTool/FireflyTool
cp "THUMB RAM Debug/FireflyUsbTest/FireflyUsbTest.elf" ../firefly-production-tools/FireflyTool/FireflyTool
cp "THUMB RAM Debug/FireflyIceTest/FireflyIceTest.elf" ../firefly-production-tools/FireflyTool/FireflyTool

cp "THUMB RAM Debug/FireflyFlash/FireflyFlash.elf" ../firefly-production-tools/FireflyProduction/FireflyProduction
