#ifndef ENCRYPT_H
#define ENCRYPT_H

#include <iostream>
#include "tomcrypt.h"

#define MAX_SIZE	512*4


//��libtomcrypt��ļ����㷨���а�װ���������ڴ�������
//ͨ��������ģʽ(CTR)�µķ�����������㷨����Ϣ��������������
class Encrypt
{
public:
	//��ʼ�������㷨�������ȵ��ã���Ҫ���������Ϊ����
	int	init(unsigned char* salt)
	{
		unsigned long	buflen;
		unsigned char	buf[32];

		//ע��ɢ�кͼ����㷨
		::register_cipher(&aes_desc);
		::register_hash(&sha256_desc);

		//����PKCS#5����chatkk��Ϊɢ�п���
		buflen = sizeof(buf);
		if(pkcs_5_alg2((const unsigned char*)"chatkk",6, salt,8, 1024, find_hash("sha256"), buf,&buflen)!=CRYPT_OK )
			return 0;
	
		//������Կ�ͼ�����
		memcpy(secretkey,buf,16);
		memcpy(IV,buf+16,16);
		return 1;
	}

	//����ָ����С������
	int	encrypt( unsigned char *plaintext, unsigned char *ciphertext, int len)
	{
		//����������ģʽ
		if(ctr_start(find_cipher("aes"),IV,secretkey,16,0,CTR_COUNTER_BIG_ENDIAN,&ctr) != CRYPT_OK)
			return 0;

		//����
		::ctr_encrypt(plaintext,ciphertext, len,&ctr);
		return 1;
	}

	//����ָ����С������
	int	decrypt( unsigned char *ciphertext, unsigned char *plaintext, int len)
	{	
		static int first=1;
		//��һ�ν�������������ģʽ(��ֹδ��������½��н���)
		if(first)
		{
			if(ctr_start(find_cipher("aes"),IV,secretkey,16,0,CTR_COUNTER_BIG_ENDIAN,&ctr) != CRYPT_OK)
				return 0;
			first = 0;
		}

		//���ü�����IV
		::ctr_setiv(IV,16,&ctr);

		//����
		::ctr_decrypt(ciphertext,plaintext, len,&ctr);
		return 1;
	}
private:
	symmetric_CTR ctr;				//�����㷨�ṹ
	unsigned char secretkey[16];	//��ȫ��Կ
	unsigned char IV[16];			//������
};

#endif