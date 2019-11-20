# PN5180 Firmware Updater Using STM32

This Project **Modify** From PN5180 Secure Firmware Update Library (SW4055) And **Copy** Some Code In Arduino Forum.

**Tested ON**

+ STM32F103C8T6 - BluePill

It Can Modify For Other STM32, Try With Yourself : )

# How To Update

1. Download NFC Cockpit And Get The PN5180 Firmware File (*.sfwu) In PN5180_SFWU Folder.
2. Convert .sfwu To C Array Of Bytes By This Website https://tomeko.net/online_tools/file_to_hex.php?lang=en
3. Save And Include As Header.
4. Connect PN5180 Module To STM32
5. Modify Pin From #define *_PIN
6. Upload The Example That Include The Firmware Header.
7. Profit !!

# THE EEPROM WILL BE WIPE BY NEW FIRMWARE !!
# IT WILL STUCK IN DOWNLOAD MODE, IF UPLOAD FAILED !!
# I WON'T RESPONSIBLE FOR ANY BRICK !!
