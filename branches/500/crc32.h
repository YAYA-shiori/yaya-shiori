#ifndef CRC32_INCLUDED
#define CRC32_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif
// �ꔭ�Ńf�[�^��񋟂ł��Ȃ��ꍇ�Acrc�p�����[�^�ɑO��̌v�Z���ʂ�n���B
// �����X�e�[�^�X��0�B�������0�o�C�g�̏ꍇ��0�B
unsigned long update_crc32(const unsigned char *buf,unsigned int len,unsigned long crc);

#ifdef __cplusplus
}
#endif

#endif //CRC32_INCLUDED

