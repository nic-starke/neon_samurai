import { ref } from "vue";
import * as AtmelDFU from './AtmelDFU.js';


/*
 * Actions
 */
let target = null; // USBDevice
export const device = ref('Not Selected');
export const status = ref('');
export const message = ref('');


export async function selectDevice() {
    try {
        // close previous device
        if (target !== null) {
            await target.close();
            target = null;
        }

        target = await AtmelDFU.getAtmelDevice();
        if (target === null) {
            device.value = 'Not selected';
            return;
        }

        device.value = target.productName;
        console.log(`Selected: ${target.productName}`);
    } catch(e) {
        target = null;
        device.value = 'Not selected';
        console.log(e);
    }
}

export async function checkStatus() {
    try {
        let result = await AtmelDFU.getStatus(target);

        status.value = result.status;
        let stat = AtmelDFU.parseStatus(result.data);
        console.log('status: '        + result.status);
        console.log('bStatus: '       + stat.bStatus);
        console.log('bwPollTimeOut: ' + stat.bwPollTimeOut);
        console.log('bState: '        + stat.bState);
        console.log('iString: '       + stat.iString);

        message.value = `status: ${stat.bStatus}, state: ${stat.bState}`;
    } catch(e) {
        message.value = `Error: ${e.name}`;
        device.value = 'Invalid';
        status.value = 'error';
        console.log(e);
    }
}

export async function eraseChip() {
    try {
        message.value = 'Erasing Flash...';
        let result = await AtmelDFU.chipErase(target);
        status.value = result.status;
        message.value = 'Done';
    } catch(e) {
        device.value = 'Invalid';
        status.value = 'error';
        console.log(e);
    }
}

export async function checkBlank() {
    try {
        let d = AtmelDFU.deviceInfo.find((e) => e.productId === target.productId);
        if (d === undefined) {
            console.log('Unknown Device');
            return;
        }

        let result = await AtmelDFU.blankCheck(target, 0, d.flashSize - 1);
        console.log('byteWritten: ' + result.bytesWritten);

        result = await AtmelDFU.getStatus(target);
        let stat = AtmelDFU.parseStatus(result.data);

        if (stat.bStatus == AtmelDFU.bStatus.OK) {
            console.log('Blank');
            message.value = 'Blank';
        } else if (stat.bStatus === AtmelDFU.bStatus.errCHECK_ERASED) {
            console.log('Not Blank');
            message.value = 'Not Blank';
        } else {
            console.log('Unknown Error');
            message.value = 'Unknown Error';
        }

        status.value = result.status;
    } catch(e) {
        device.value = 'Invalid';
        status.value = 'error';
        console.log(e);
    }
}

function hexStr(n, digit=2, toUpper=true) {
    let s = n.toString(16).padStart(digit, '0');
    if (toUpper)
        return s.toUpperCase();
    else
        return s;
}

function loadHex(text) {
    // https://en.wikipedia.org/wiki/Intel_HEX
    let data = [];
    let ext_addr = 0;
    let processed = 0;
    let lines = text.split(/\r?\n/);
    for (let index = 0; index < lines.length; index++) {
        //   DATA:  ':'   dlen    addr    '00'  data        chkSum
        //    EOF:  ':'  '00'     '0000'  '01'              chkSum
        //    EXT:  ':'  '02'     '0000'  '02'  address     chkSum
        //   segm:  ':'  '04'     '0000'  '03'  CS+IP       chkSum
        // hiaddr:  ':'  '02'     '0000'  '04'  address     chkSum
        //   addr:  ':'  '04'     '0000'  '05'  address     chkSum
        if (lines[index].length == 0) {
            continue;
        }

        let line = lines[index].trim();
        if (line.length < 11)   { throw new Error(`Invalid at line ${index + 1}`) }
        if (line.at(0) !== ':') { throw new Error(`Invalid at line ${index + 1}`) }

        let bytes = line.slice(1).match(/[0-9a-fA-F]{2}/g).map((h) => parseInt(h,16));

        let checkSum = bytes.pop();
        let sum = bytes.reduce((a, c) => a + c, 0);
        if (checkSum !== (-sum & 0xff)) { throw new Error(`Checksum error at line ${index + 1}`) }

        let dlen = bytes.shift();
        let addr = bytes.shift() << 8 | bytes.shift();
        let type = bytes.shift();

        switch (type) {
            case 0: // DATA
                if (data.length < ext_addr + addr) {
                    //console.log(`loadHex: skip from: ${data.length}, to: ${ext_addr + addr}`);
                    for (let i = data.length; i < ext_addr + addr; i++) {
                        data.push(0xff); // blank
                    }
                } else if (data.length > ext_addr + addr) {
                    throw new Error(`Address error at line ${index + 1}`);
                }
                data.push(...bytes);
                break;
            case 1: // EOF
                // should stop?
                //console.log(`loadHex: EOF at line ${index + 1}`);
                break;
            case 2: // Extended Segment Address
                if (2 !== bytes.length) throw new Error(`Invalid record at line ${index + 1}`);
                ext_addr = ((bytes[0] << 8) | bytes[1]) * 16;
                console.log(`loadHex: Extended Segment Address: ${ext_addr} at line ${index + 1}`);
                break;
            case 4: // Extended Linear Address
                if (2 !== bytes.length) throw new Error(`Invalid record at line ${index + 1}`);
                ext_addr = ((bytes[0] << 8) | bytes[1]) << 16;
                console.log(`loadHex: Extended Segment Address: ${ext_addr} at line ${index + 1}`);
                break;

            case 3: // Start Segment Address
            case 5: // Start Linear Address
                console.log(`loadHex: Not supported record type ${type} at line ${index + 1}`);
                break;

            default:
                throw new Error(`loadHex: Invalid record type ${type} at line ${index + 1}`);
        }
        processed++;
    }
    return data;
}

export async function programFlash() {
    try {
        // read hex file
        let hexFile = document.querySelector("#hexFile");
	if (hexFile.files.length == 0) {
            message.value = 'No file is specified.';
            return;
        }
        let text = await hexFile.files[0].text();
        let data = loadHex(text);

        // close previous device
        if (target !== null) {
            await target.close();
            target = null;
        }
        target = await AtmelDFU.getAtmelDevice();
        if (target === null) {
            return;
        }
        device.value = target.productName;
        console.log(`Selected: ${target.productName}`);

        // find device info
        let d = AtmelDFU.deviceInfo.find((e) => e.productId === target.productId);
        if (d === undefined) {
            message.value = 'Unknown Device';
            return;
        }
        if (data.length > d.flashSize) {
            message.value = 'Specified firmware is larger than Flash space.';
            return;
        }

        message.value = 'Erasing Flash...';
        let result = await AtmelDFU.chipErase(target);

        message.value = 'Writing Flash...';
        result = await AtmelDFU.writeMemory(target, 0x0000, data.length - 1, data);

        message.value = 'Verifing...';
        let mem = await AtmelDFU.readMemory(target, 0, data.length - 1);

        for (let i = 0; i < mem.byteLength; i++) {
            if (mem[i] !== data[i]) {
                console.log(hexStr(i, 4) + ': ' + hexStr(mem[i]));
                message.value = `Failed to verify at ${hexStr(i, 4)}`;
                return;
            }
        }

        message.value = 'Launching...';
        result = await AtmelDFU.launch(target);
        status.value = result.status;

        message.value = `Done. Wrote Flash ${data.length} bytes.`;
    } catch (e) {
        message.value = 'Error on programming flash';
        status.value = 'Error on programming flash';
        console.log(e);
    } finally {
        if (target !== null) {
            await target.close();
        }
        target = null;
        device.value = 'Not selected';
    }
}

export async function writeFlash() {
    try {
        // read hex file
        let hexFile = document.querySelector("#flashFile");
	if (hexFile.files.length == 0) {
            message.value = 'No file is specified.';
            return;
        }
        let text = await hexFile.files[0].text();
        let data = loadHex(text);

        // write
        message.value = 'Writing Flash...';
        let result = await AtmelDFU.writeMemory(target, 0x0000, data.length - 1, data);

        // verify
        message.value = 'Verifing...';
        let mem = await AtmelDFU.readMemory(target, 0, data.length - 1);

        for (let i = 0; i < mem.byteLength; i++) {
            if (mem[i] !== data[i]) {
                console.log(hexStr(i, 4) + ': ' + hexStr(mem[i]));
                message.value = `Failed to verify at ${hexStr(i, 4)}`;
                return;
            }
        }
        message.value = `Done. Wrote Flash ${data.length} bytes.`;
    } catch (e) {
        message.value = 'Error on writing flash';
        status.value = 'Error on writing flash';
        console.log(e);
        console.log(e.message);
        console.log(e.name);
    }
}

export async function readFlash() {
    try {
        let d = AtmelDFU.deviceInfo.find((e) => e.productId === target.productId);
        if (d === undefined) {
            console.log('Unknown Device');
            return;
        }

        let end = d.flashSize - 1;
        //let end = d.flashSize - d.bootSize;

        message.value = 'Reading Flash...';
        let mem = await AtmelDFU.readMemory(target, 0, end, false);
        message.value = 'Done';

        // download link
        let link = document.querySelector('#flash-link');
        let button = document.querySelector('#flash-download');
        button.addEventListener("click", (e) => { link.click(); });

        window.URL.revokeObjectURL(link.getAttribute("href"));
        let blob = new Blob([mem], { type: "image/jpeg" });
        let file =  window.URL.createObjectURL(blob);
        link.setAttribute("href", file);
        link.setAttribute("download", "flash.bin");
        button.removeAttribute("disabled");
    } catch (e) {
        console.log(e);
    }
}

export async function clearEEPROM() {
    try {
        let d = AtmelDFU.deviceInfo.find((e) => e.productId === target.productId);
        if (d === undefined) {
            console.log('Unknown Device');
            return;
        }
        let end = d.eepromSize - 1;

        let data = new Uint8Array(d.eepromSize);
        data.fill(0xff, 0, d.eepromSize);

        // write
        message.value = 'Writing EEPROM...';
        let result = await AtmelDFU.writeMemory(target, 0, end, data, true);

        // verify
        message.value = 'Verifing...';
        let mem = await AtmelDFU.readMemory(target, 0, end, true);

        for (let i = 0; i < mem.byteLength; i++) {
            if (mem[i] !== data[i]) {
                console.log(hexStr(i, 4) + ': ' + hexStr(mem[i]));
                message.value = `Failed to verify at ${hexStr(i, 4)}`;
                return;
            }
        }
        message.value = `Done. Wrote EEPROM ${data.length} bytes.`;
    } catch (e) {
        message.value = 'Error on writing EEPROM';
        status.value = 'Error on writing EEPROM';
        console.log(e);
        console.log(e.message);
        console.log(e.name);
    }
}

export async function writeEEPROM() {
    try {
        let d = AtmelDFU.deviceInfo.find((e) => e.productId === target.productId);
        if (d === undefined) {
            console.log('Unknown Device');
            return;
        }
        let end = d.eepromSize - 1;

        // read hex file
        let hexFile = document.querySelector("#eepromFile");
	if (hexFile.files.length == 0) {
            message.value = 'No file is specified.';
            return;
        }
        let text = await hexFile.files[0].text();
        let data = loadHex(text);

        // write
        message.value = 'Writing EEPROM...';
        let result = await AtmelDFU.writeMemory(target, 0, data.length - 1, data, true);

        // verify
        message.value = 'Verifing...';
        let mem = await AtmelDFU.readMemory(target, 0, data.length - 1, true);

        for (let i = 0; i < mem.byteLength; i++) {
            if (mem[i] !== data[i]) {
                console.log(hexStr(i, 4) + ': ' + hexStr(mem[i]));
                message.value = `Failed to verify at ${hexStr(i, 4)}`;
                return;
            }
        }
        message.value = `Done. Wrote EEPROM ${data.length} bytes.`;
    } catch (e) {
        message.value = 'Error on writing EEPROM';
        status.value = 'Error on writing EEPROM';
        console.log(e);
        console.log(e.message);
        console.log(e.name);
    }
}

export async function readEEPROM() {
    try {
        let d = AtmelDFU.deviceInfo.find((e) => e.productId === target.productId);
        if (d === undefined) {
            console.log('Unknown Device');
            return;
        }
        let end = d.eepromSize - 1;

        message.value = 'Reading EEPROM...';
        let mem = await AtmelDFU.readMemory(target, 0, end, true);
        message.value = 'Done';

        // download link
        let link = document.querySelector('#eeprom-link');
        let button = document.querySelector('#eeprom-download');
        button.addEventListener("click", (e) => { link.click(); });

        window.URL.revokeObjectURL(link.getAttribute("href"));
        let blob = new Blob([mem], { type: "image/jpeg" });
        let file =  window.URL.createObjectURL(blob);
        link.setAttribute("href", file);
        link.setAttribute("download", "eeprom.bin");
        button.removeAttribute("disabled");
    } catch (e) {
        console.log(e);
    }
}

export async function startApp() {
    try {
        let result = await AtmelDFU.launch(target);
        status.value = result.status;   // 'ok' or 'stall'
    } catch(e) {
        device.value = 'Invalid';
        status.value = 'error';
        console.log(e);
    } finally {
        target = null;
        device.value = 'Not selected';
    }
}

export async function recoverError() {
    try {
        let result = await AtmelDFU.clearStatus(target);
        if (result.status !== 'ok') {
            console.log('aborting...');
            result = await abort(target);
        }
        status.value = result.status;
        console.log('status: ' + result.status);
    } catch(e) {
        device.value = 'Invalid';
        status.value = 'error';
        console.log(e);
    }
}
