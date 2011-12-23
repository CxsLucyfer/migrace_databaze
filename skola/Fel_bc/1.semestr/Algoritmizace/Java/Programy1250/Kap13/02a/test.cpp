/* Zdrojov� text programu v C++, kter� vytvo�� bin�rn� soubor
 * a zap�e do n�j cel� ��sla (typ int) od 0 do 9.
 * Slou�� jak� p��klad na bin�rn�n nekompatibilitu datov�ch soubor� vytvo�en�ch
 * v Jav� a v jin�ch programovac�ch jazyc�ch na PC.
 */

// P�ecti si informace o standardn�ch prost�edc�ch pro vstup a v�stup

#include <stdio.h>
void main()								
{
 FILE * f = fopen("data.dta", "wb"); // Otev�i bin�rn� pro z�pis
 for(int i = 0; i < 10; i++)			
	fwrite(&i, sizeof(int),1, f);// V�pis ��sla 0 a� 10 typu int
 fclose(f);                          // Uzav�i soubor
}