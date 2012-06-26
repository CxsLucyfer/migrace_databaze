/* Soubor Kap14\04\vokno\Vokno.java 
 * Ukazuje vytvo�en� komponenty (tla��tka) a 
 * jeho um�st�n� do okna pomoc� spr�vce um�st�n� Borderlayout.
 * Lze snadno upravit a pou��t jin� spr�vce um�st�n� (tak vznikly 
 * odpov�daj�c� obr�zky v knize).
 * Pro v�echny verze JDK 
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
		getContentPane().setLayout(bl);
		getContentPane().add(tlacitko1, BorderLayout.CENTER);
		getContentPane().add(tlacitko2, BorderLayout.NORTH);
		getContentPane().add(tlacitko3, BorderLayout.WEST);
		getContentPane().add(tlacitko4, BorderLayout.SOUTH);
		getContentPane().add(tlacitko5, BorderLayout.EAST);
	} 
	
}
