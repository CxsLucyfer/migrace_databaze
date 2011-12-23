/* Soubor Kap14\06\Piskorky.java
 * Hlavn� t��da programu Pi�korky
 * Okno tohoto programu slou�� jako hrac� plocha, slo�en� z pol��ek, na kter� se 
 * kliknut�m my�� zaznamen�vaj� kole�ka nebo k��ky. Hr��i se v taz�ch st��daj�, 
 * c�lem je vytvo�it "pi�k(v)orku" - p�t stejn�ch symbol� za sebou v �ad�, 
 * sloupci nebo na diagon�le.
 *
 * Hlavn� program pouze vytvo�� okno a um�st� ho doprost�ed obrazovky, nic v�c.
 */

import javax.swing.*;
import java.awt.*;
import vokno.Vokno;

public class Piskorky
{
	public Piskorky()
	{
		Vokno okno = new Vokno();

		Dimension obr = Toolkit.getDefaultToolkit().getScreenSize();
		okno.setSize(new Dimension(obr.width/2, obr.height/2+50));
		okno.setLocation(obr.width/4, obr.height/4);

		okno.setVisible(true);
		okno.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
	}

	public static void main(String[] a)
	{	
		try {
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		}
		catch(Exception e) {
			e.printStackTrace();
		}
		Piskorky p = new Piskorky();
	}
}