#include "fru.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define FRU_VERSION 1
#define BOARD_AREA_VERSION 1
#define PRODUCT_AREA_VERSION 1
#define FRU_SIZE 4096
#define TAG "FRU"

#ifndef log
#include "common.h"
//#define log(...) {printf ("L["TAG"]: "__VA_ARGS__); }
//#define warn(...) {printf ("W["TAG"]: "__VA_ARGS__); }
//#define err(...) {printf ("E["TAG"]: "__VA_ARGS__); }
#endif

#define MR_MAC_REC 0xC0
#define MR_UBOOT_REC 0xC1

static uint8_t fru_buf[FRU_SIZE];
struct fru fru;

uint8_t
calc_cs(uint8_t *buf, uint8_t size) {
  uint8_t cs = 0;
  int i = 0;
  fprintf(logfile, "CS:\n");
  for (;i<size; i++) {
    cs += buf[i];
    if (i%8==0) {
      fprintf(logfile, "\n");
    }
    fprintf(logfile, "0x%02x ", cs);
  }
  fprintf(logfile, "\n");
  return cs;
}

int
read_fru_str(uint8_t *buf, uint8_t *str, unsigned int *len, unsigned int offt) {
  *len = ((buf[offt]>(FRU_STR_MAX-1))?(FRU_STR_MAX-1):buf[offt]);
  memset(str, 0, FRU_STR_MAX);
  memcpy(str, &buf[offt+1], *len);
  offt += buf[offt]+1;
  return offt;
}

int
parse_board_area(struct fru *f, uint8_t *buf, unsigned int buf_len) {
  uint8_t cs;
  int offt = 0;
  int i = 0;
  
  if (buf[0] != BOARD_AREA_VERSION) {
    warn("FRU: Board area version is not valid\n");
    return -1;
  }
  if ((buf[1]*8)>buf_len) {
    warn("FRU: Board area size mismatch\n");
    return -2;
  }
  log("FRU Board area bin:\n");
  for (;i<buf[1]*8;i++) {
    if ((i%8)==0) {
      fprintf(logfile, "\n");
    }
    fprintf(logfile, "0x%02x[%c] ", buf[i], (buf[i]>' '?buf[i]:' '));
  }
  fprintf(logfile, "\n");

  cs = calc_cs(buf, buf[1]*8);
  if (cs != 0) {
    warn("FRU: Bad board area checksum [0-%i]: %i\n", buf[1]*8, cs);
    return -3;
  }
  f->mfg_date[0] = buf[3];
  f->mfg_date[1] = buf[4];
  f->mfg_date[2] = buf[5];

  offt = 5;
  offt = read_fru_str(buf, f->val_mfg_name, &f->len_mfg_name, offt);
  offt = read_fru_str(buf, f->val_product_name, &f->len_product_name, offt);
  offt = read_fru_str(buf, f->val_serial_number, &f->len_serial_number, offt);
  offt = read_fru_str(buf, f->val_part_number, &f->len_part_number, offt);
  offt = read_fru_str(buf, f->val_fru_id, &f->len_fru_id, offt);
  log("FRU Board area:\n");
  log("Board mfg:\t\t\t%s\n", f->val_mfg_name);
  log("Board name:\t\t\t%s\n", f->val_product_name);
  log("Board serial number:\t\t\t%s\n", f->val_serial_number);
  log("Board part number:\t\t\t%s\n", f->val_part_number);
  log("Board fru id:\t\t\t%s\n", f->val_fru_id);
  return 0;
}

int
parse_product_area(struct fru *f, uint8_t *buf, unsigned int buf_len) {
  int offt = 0;
  if (buf[0] != PRODUCT_AREA_VERSION) {
    warn("FRU: Product area version is not valid\n");
    return -1;
  }
  if ((buf[1]*8)>buf_len) {
    warn("FRU: Product area size mismatch\n");
    return -2;
  }
  if (calc_cs(buf, buf[1]*8) != 0) {
    warn("FRU: Bad product area checksum\n");
    return -3;
  }
  offt = 3;
  offt = read_fru_str(buf, f->val_p_product_mfg, &f->len_p_product_mfg, offt);
  offt = read_fru_str(buf, f->val_p_product_name, &f->len_p_product_name, offt);
  offt = read_fru_str(buf, f->val_p_part_model_number, &f->len_p_part_model_number, offt);
  offt = read_fru_str(buf, f->val_p_product_version, &f->len_p_product_version, offt);
  offt = read_fru_str(buf, f->val_p_serial_number, &f->len_p_serial_number, offt);
  offt = read_fru_str(buf, f->val_p_fru_id, &f->len_p_fru_id, offt);
  log("FRU Product area:\n");
  log("Product mfg:\t\t\t%s\n", f->val_p_product_mfg);
  log("Product name:\t\t\t%s\n", f->val_p_product_name);
  log("Product model number:\t\t\t%s\n", f->val_p_part_model_number);
  log("Product version:\t\t\t%s\n", f->val_p_product_version);
  log("Product serial number:\t\t\t%s\n", f->val_p_serial_number);
  log("Product fru id:\t\t\t%s\n", f->val_p_fru_id);
  return 0;
}

int
fru_parse_multirecord(struct multirec *m, uint8_t *buf, unsigned int buf_len) {
  int i = 0;
  uint8_t data_cs;
  if (buf_len<5) {
    warn("FRU: no space in multirecord buffer, failed to parse header\n");
    return -4;
  }
  if (calc_cs(buf, 5) != 0) {
    warn("FRU: multirecord header checksum is invalid\n");
    m->header_cs_ok = false;
    return -1;
  } else {
    m->header_cs_ok = true;
  }
  m->type = buf[0];
  m->format = buf[1]&0x7;
  if (m->format != 0x2) {
    warn("FRU: multirecord format is unknown [%i]\n", m->format);
    return -2;
  }
  m->end = (buf[1]&0x08?true:false);
  if (m->end) {
    log("FRU: last multirecord\n");
  }
  m->length = buf[2]*8;
  if ((buf_len-5)<(m->length)) {
    warn("FRU: no space in multirecord buffer, failed check data\n");
    return -5;
  }
  log("FRU mrec bin:\n");
  for (;i<(m->length+5);i++) {
    if ((i%8)==0) {
      fprintf(logfile, "\n");
    }
    fprintf(logfile, "0x%02x[%c] ", buf[i], (buf[i]>' '?buf[i]:' '));
  }
  fprintf(logfile, "\n");

  data_cs = calc_cs(&buf[5], m->length)+buf[3];
  if (data_cs != 0) {
    warn("FRU: multirecord data checksum is invalid [0x%02x]\n", data_cs);
    return -3;
  }
  m->data = &buf[5];
  return m->length+5;
}

int
parse_fru(struct fru *f, uint8_t *buf, unsigned int buf_len) {
  int ret = 0;
  int mrec_n = 0;
  unsigned int board_area_offset;
  unsigned int product_area_offset;
  unsigned int mrec_area_offset;

  if (buf_len<8) {
    warn("FRU buffer is too short\n");
    return -1;
  }
  log("FRU: Checking header [0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x]\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
  if (buf[0] == 0xff) {
    warn("FRU: Empty EEPROM detected\n");
    return -2;
  } else if (buf[0] != FRU_VERSION) {
    warn("FRU: Header version is not valid\n");
    return -3;
  } else if (calc_cs(buf, 8) != 0) {
    warn("FRU: Bad header checksum: %i\n", calc_cs(buf, 8));
    return -4;
  }
  board_area_offset = buf[3]*8;
  product_area_offset = buf[4]*8;
  mrec_area_offset = buf[5]*8;
  if (parse_board_area(f, &buf[board_area_offset], buf_len-board_area_offset)) {
    return -5;
  }
  if (parse_product_area(f, &buf[product_area_offset], buf_len-product_area_offset)) {
    return -6;
  }
  f->mrec_count = 0;
  while (ret >= 0) {
    log("FRU: parsing multirecord %i\n", f->mrec_count);
    ret = fru_parse_multirecord(&f->mrec[mrec_n], &buf[mrec_area_offset], buf_len-mrec_area_offset);
    if (ret > 0) {
      f->mrec_count ++;
    }
    if ((ret > 0) && (f->mrec[mrec_n].end == false)) {
      mrec_area_offset = ret;
    } else {
      break;
    }
  }
  return 0;
}

int
fru_open_parse(void) {
  FILE *f = fopen("/sys/bus/i2c/devices/1-0053/eeprom", "r");
  int ret = 0;
  int i = 0;
  if (f == NULL) {
    err("FRU: failed to open eeprom\n");
    return -1;
  }
  log("Reading eeprom\n");
  ret = fread(fru_buf, sizeof(uint8_t), FRU_SIZE, f);
  log("Read %i bytes\n", ret);
  parse_fru(&fru, fru_buf, FRU_SIZE);
  for (i=0; i<fru.mrec_count; i++) {
    if (fru.mrec[i].type == MR_MAC_REC) {
      memcpy(fru.mac, fru.mrec[i].data, 6);
      log("FRU: found MAC mrec [%02x %02x %02x %02x %02x %02x]\n", fru.mac[0], fru.mac[1], fru.mac[2], fru.mac[3], fru.mac[4], fru.mac[5]);
      break;
    }
  }
  fclose(f);
  return 0;
}
