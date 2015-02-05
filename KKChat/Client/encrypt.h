#ifndef ENCRYPT_H
#define ENCRYPT_H

#include <iostream>
#include "tomcrypt.h"

#define MAX_SIZE	512*4


//对libtomcrypt库的加密算法进行包装，以适用于此聊天室
//通过计数器模式(CTR)下的分组密码加密算法对消息进行数据流加密
class Encrypt
{
public:
	//初始化加密算法，必须先调用，需要以随机数作为输入
	int	init(unsigned char* salt)
	{
		unsigned long	buflen;
		unsigned char	buf[32];

		//注册散列和加密算法
		::register_cipher(&aes_desc);
		::register_hash(&sha256_desc);

		//调用PKCS#5，以chatkk作为散列口令
		buflen = sizeof(buf);
		if(pkcs_5_alg2((const unsigned char*)"chatkk",6, salt,8, 1024, find_hash("sha256"), buf,&buflen)!=CRYPT_OK )
			return 0;
	
		//保存密钥和计数器
		memcpy(secretkey,buf,16);
		memcpy(IV,buf+16,16);
		return 1;
	}

	//加密指定大小的数据
	int	encrypt( unsigned char *plaintext, unsigned char *ciphertext, int len)
	{
		//启动计数器模式
		if(ctr_start(find_cipher("aes"),IV,secretkey,16,0,CTR_COUNTER_BIG_ENDIAN,&ctr) != CRYPT_OK)
			return 0;

		//加密
		::ctr_encrypt(plaintext,ciphertext, len,&ctr);
		return 1;
	}

	//解密指定大小的数据
	int	decrypt( unsigned char *ciphertext, unsigned char *plaintext, int len)
	{	
		static int first=1;
		//第一次解密启动计数器模式(防止未启动情况下进行解密)
		if(first)
		{
			if(ctr_start(find_cipher("aes"),IV,secretkey,16,0,CTR_COUNTER_BIG_ENDIAN,&ctr) != CRYPT_OK)
				return 0;
			first = 0;
		}

		//重置计数器IV
		::ctr_setiv(IV,16,&ctr);

		//解密
		::ctr_decrypt(ciphertext,plaintext, len,&ctr);
		return 1;
	}
private:
	symmetric_CTR ctr;				//加密算法结构
	unsigned char secretkey[16];	//安全密钥
	unsigned char IV[16];			//计数器
};

#endif