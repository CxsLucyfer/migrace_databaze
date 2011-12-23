/* Soubor Kap13\02a\Test.java
 * Uk�zka bin�rn� nekompatibility mezi C++ a Javou na PC
 * 
 * Nejprve spus�te pod Windows program Test.exe. To je p�elo�en� program,
 * napsan� v C++, kter� vytvo�� bin�rn� soubor data.dta a zap�e do n�j ��sla
 * 0, 1, ... , 9. 
 * (Zdrojov� text programu v C++ je v souboru Test.cpp. Zn�te-li Javu, 
 * nebude pro v�s probl�m mu porozum�t.)
 * Pak spus�te tento program, kter� se ona ��sla pokus� p�e��st. 
 * Vyp�e jin� ��sla.
 *
 * D�vodem je bin�rn� nekompatibilita - C++, C , Pascal a jin� programovac� 
 * jazyky u��vaj� "p�irozen�" form�t na dan�m po��ta�i, zat�mco Java 
 * m� form�t pevn� stanoven�, kter� je stejn� na v�ech po��ta��ch.
 * NA PC SE P�IROZEN� FORM�T NESHODUJE S FORM�TEM POU��VAN�M JAVOU.
 */

import java.io.*;

class Test	// P�e�te 10 ��sel typu int z bin�rn�ho souboru a vyp�e je
{		// P�edpokl�d� existenci bin�rn�ho souboru data.dta
	public static void main(String[] a) throws Exception
	{
		FileInputStream fis = new FileInputStream("data.dta");
		DataInputStream dis = new DataInputStream(fis);
		int x;
		for(int i = 0; i < 10; i++)
		{
			System.out.println(dis.readInt());
		}
	}
}