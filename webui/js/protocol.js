// Per-param GET/SET orchestration on top of midi.js/sysex.js. Mirrors
// tests/robot/lib/NeonSamuraiLibrary.py's keyword shape so the two
// independent clients stay easy to diff when the protocol changes.
//
// GET requests must carry a dummy payload sized to the param's real wire
// struct: the firmware validates packet length and NAKs by dropping the
// message, so an undersized payload surfaces only as a timeout. GET replies
// carry the param's data alone, with no echo of the index prefix.
//
// Only the two setters the live view actually uses are here. HSV is the
// device's writable colour source of truth and RGB a derived read-only
// mirror, so an editing UI would add setVmapHsv rather than setVmapRgb.

import {
  Cmd,
  Param,
  activeBankPayload,
  encoderPayload,
  livePositionStreamPayload,
  sideSwitchPayload,
  vmapCurrPosPayload,
  vmapHsvPayload,
  vmapPositionPayload,
  vmapRangePayload,
  vmapRbPayload,
  vmapRgbPayload,
} from "./sysex.js";

export class Protocol {
  constructor(device) {
    this.device = device;
  }

  async getDeviceInfo() {
    const reply = await this.device.request(Cmd.GET, Param.DEVICE_INFO);
    const d = reply.data;
    return {
      fwVersion: `${d[0]}.${d[1]}.${d[2]}`,
      numEncoders: d[3],
      numBanks: d[4],
      numVmapsPerEncoder: d[5],
      numSideSwitches: d[6],
    };
  }

  async getEncoderParam(param, bank, enc) {
    const reply = await this.device.request(
      Cmd.GET,
      param,
      encoderPayload(bank, enc, 0)
    );
    return reply.data[0];
  }

  async getVmapRange(bank, enc, vmap) {
    const reply = await this.device.request(
      Cmd.GET,
      Param.VMAP_RANGE,
      vmapRangePayload(bank, enc, vmap, 0, 0)
    );
    return {
      lower: toSigned16(reply.data[0] | (reply.data[1] << 8)),
      upper: toSigned16(reply.data[2] | (reply.data[3] << 8)),
    };
  }

  async getVmapPosition(bank, enc, vmap) {
    const reply = await this.device.request(
      Cmd.GET,
      Param.VMAP_POSITION,
      vmapPositionPayload(bank, enc, vmap, 0, 0)
    );
    return { start: reply.data[0], stop: reply.data[1] };
  }

  async getVmapHsv(bank, enc, vmap) {
    const reply = await this.device.request(
      Cmd.GET,
      Param.VMAP_HSV,
      vmapHsvPayload(bank, enc, vmap, 0, 0, 0)
    );
    const [lo, hi, sat, val] = reply.data;
    return { hue: lo | (hi << 8), sat, val };
  }

  async getVmapRgb(bank, enc, vmap) {
    const reply = await this.device.request(
      Cmd.GET,
      Param.VMAP_RGB,
      vmapRgbPayload(bank, enc, vmap, 0, 0, 0)
    );
    return { r: reply.data[0], g: reply.data[1], b: reply.data[2] };
  }

  async getVmapRb(bank, enc, vmap) {
    const reply = await this.device.request(
      Cmd.GET,
      Param.VMAP_RB,
      vmapRbPayload(bank, enc, vmap, 0, 0)
    );
    return { r: reply.data[0], b: reply.data[1] };
  }

  // struct proto_cfg is 4 bytes on the wire - type + mode + channel +
  // cc-or-raw - under -fpack-struct -fshort-enums.
  async getVmapProto(bank, enc, vmap) {
    const reply = await this.device.request(Cmd.GET, Param.VMAP_PROTO, [
      bank,
      enc,
      vmap,
      0,
      0,
      0,
      0,
    ]);
    const [type, mode, channel, ccOrRaw] = reply.data;
    return { type, mode, channel, ccOrRaw };
  }

  async getSideSwitch(swIdx) {
    const reply = await this.device.request(
      Cmd.GET,
      Param.SIDE_SWITCH,
      sideSwitchPayload(swIdx, 0)
    );
    return reply.data[0];
  }

  async getVmapCurrPos(bank, enc, vmap) {
    const reply = await this.device.request(
      Cmd.GET,
      Param.VMAP_CURR_POS,
      vmapCurrPosPayload(bank, enc, vmap, 0)
    );
    return reply.data[0];
  }

  async getActiveBank() {
    const reply = await this.device.request(
      Cmd.GET,
      Param.ACTIVE_BANK,
      activeBankPayload(0)
    );
    return reply.data[0];
  }

  async setActiveBank(bank) {
    const reply = await this.device.request(
      Cmd.SET,
      Param.ACTIVE_BANK,
      activeBankPayload(bank)
    );
    return reply.data[0];
  }

  async setLivePositionStreaming(enabled) {
    const reply = await this.device.request(
      Cmd.SET,
      Param.ENCODER_LIVE_POSITION_STREAM,
      livePositionStreamPayload(enabled)
    );
    return reply.data[0];
  }
}

// virtmap.range.{lower,upper} are i16 in firmware - widened from i8 so a
// 14-bit CC range (0-16383) is expressible.
function toSigned16(word) {
  return word > 32767 ? word - 65536 : word;
}
