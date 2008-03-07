#ifndef CRC32_INCLUDED
#define CRC32_INCLUDED

// 一発でデータを提供できない場合、crcパラメータに前回の計算結果を渡す。
// 初期ステータスは0。もちろん0バイトの場合も0。
unsigned long update_crc32(const unsigned char *buf,unsigned int len,unsigned long crc);

#endif //CRC32_INCLUDED

