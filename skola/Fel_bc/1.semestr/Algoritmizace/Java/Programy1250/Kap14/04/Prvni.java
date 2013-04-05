/* Soubor Kap14\04\Prvni.java
 * Vytvo�� okno a nastav� vzhled ("Look and Feel")
 * T��da Vokno ukazuje vytvo�en� tla��tek (JButton) a jejich um�st�n� 
 * s pou�it�m spr�vce um�st�n� (Layout Manager).
 */ 

//import java.awt.*;
import javax.swing.*;

public class Prvni
{
	public static void main(String[] a)
	{
		try {
			//UIManager.setLookAndFeel("javax.swing.plaf.motif.MotifLookAndFeel");
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		}
		catch(Exception e){
			e.printStackTrace();
		}
		
		vokno.Vokno okno = new vokno.Vokno();
		okno.setVisible(true);
		okno.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
	}
}