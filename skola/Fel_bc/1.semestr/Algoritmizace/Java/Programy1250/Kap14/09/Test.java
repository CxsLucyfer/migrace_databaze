/* Soubor Kap14\09\Test.java
 * Testuje, jak je to se sou�adnicemi v okn�
 * Zobraz� okno, ve kter�m jsou t�i tla��tka
 * a textov� pole.
 * Okno m� spr�vce rozlo�en� null.
 * Stisknut� n�e polo�en�ho tla��tka zp�sob� 
 * zmen�en� hodnot prom�nn�ch x a y,
 * p�esunut� lev�ho horn�ho rohu druh�ho tla��tka do bodu (x, y)
 * a v�pis t�chto sou�adnic v textov�m poli.
 * T�et� tla��tko funguje podobn�, pouze zv�t�uje x a y
 * Neodpov�d� ��dn�mu p��kladu z textu knihy
 */

import javax.swing.*;

public class Test {
	Test()
	{
		Okno okno = new Okno();
		okno.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		okno.setVisible(true);
	}
	public static void main(String[] s)
	{
		new Test();
	}
}