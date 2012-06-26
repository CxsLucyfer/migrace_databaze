/* Soubor Kap14\04a\vokno\Vokno.java 
 * Ukazuje vytvo�en� komponenty (tla��tka) a 
 * jeho um�st�n� do okna pomoc� spr�vce um�st�n� Borderlayout
 * a vyu��v� p�itom "delegov�n�" -- v JDK 1.5 nen� t�eba
 * z�sk�vat ContentPane, sta�� vkl�dat komponenty 
 * p��mo do okna a okno se o to postar� samo
 *
 * Pro JDK 5 a vy���
 */
package vokno;
import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class Vokno extends JFrame {

	BorderLayout bl = new BorderLayout(20, 10);
	JButton tlacitko1 = new JButton();
	JButton tlacitko2 = new JButton();
	JButton tlacitko3 = new JButton();
	JButton tlacitko4 = new JButton();
	JButton tlacitko5 = new JButton();
  
	public Vokno() {		
		// enableEvents(AWTEvent.WINDOW_EVENT_MASK); // JDK 1.0
		try {
			jbInit();
		}
		catch(Exception e) {
			e.printStackTrace();
		}
	}

	private void jbInit() throws Exception
	{
		Dimension obrazovka = Toolkit.getDefaultToolkit().getScreenSize();
		Dimension ro = new Dimension();
		ro.height = obrazovka.height/2;
		ro.width = obrazovka.width/2;
		setSize(ro);
		setLocation(ro.width/2, ro.height/2);
		setTitle("Na�e prvn� okno s tla��tky, BorderLayout");
		tlacitko1.setText("�lus");
		tlacitko2.setText("Sko� do pole");
		tlacitko3.setText("Zmodrej");
		tlacitko4.setText("Zezelenej");
		tlacitko5.setText("Ty mn� taky");

                // Toto lze jen v JDK 5 a vy���ch
		setLayout(bl);
		add(tlacitko1, BorderLayout.CENTER);
		add(tlacitko2, BorderLayout.NORTH);
		add(tlacitko3, BorderLayout.WEST);
		add(tlacitko4, BorderLayout.SOUTH);
		add(tlacitko5, BorderLayout.EAST);
	} 
	
}
