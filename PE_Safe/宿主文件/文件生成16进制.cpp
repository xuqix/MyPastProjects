#include <iostream>
#include <fstream>
#include <string>

using namespace std;

int main()
{
	ifstream	in("Test.exe", ios::binary);
	ofstream	out("1.txt", ofstream::app);
	unsigned char	ch;
	char	fmt[]="0x%02x, ";
	char	buf[16];
	string	str;
	int n=0;
	if (in && out)
	{
	while(in.read((char*)&ch, 1) )
	{
		n++;
		memset(buf,0,16);
		sprintf(buf, fmt, ch);
		str	= string(buf);
		out<<str;
		if (n==16)
		{
			n=0;
			out << endl;
		}
	}
	}
	in.close();
	out.close();
	return 0;
}