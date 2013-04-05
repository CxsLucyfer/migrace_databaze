/* Soubor Kap14\08\OknoVoleb.java
 * Okno voleb pro program Pi�korky 2.0
 * Definuje konstanty ud�vaj�c� rozmez� po�tu tla��tek
 * Umo��uje zadat nejm�n� MINIMUM tla��tek, nejv��e HORNI_MEZ
 * Kombinovan� seznam nab�z� hodnoty od DOLNI_MEZ po HORNI_MEZ
 * Okno se po uzav�en� nezru��, pouze skryje
 */

package vokno;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;

public class OknoVoleb extends JDialog
{                                             // Tla��tek je v�dy n x n
  public static final int DOLNI_MEZ = 10;     // Doln� mez hodnot zobrazovan�ch v kombu
  public static final int HORNI_MEZ = 20;     // Horn� mez hodnot zobrazovan�ch v kombu
  public static final int MINIMUM = 5;        // M�n� tla��tek nem� smysl
  private boolean stiskOK = false;            // Uzav�el u�ivatel okno stiskem OK?
  Object[] volby = null;                      // Pole voleb
  Object[] rozmery = null;                    // Pole hodnot pro kombo
  JButton ok = new JButton();                 // Tla��tka
  JButton storno = new JButton();
  JTextField obrazek1 = new JTextField();     // Edita�n� pole
  JTextField obrazek2 = new JTextField();
  JComboBox kombo = null;                     // Kombo (kombinovan� seznam)
  JLabel canc1 = new JLabel();                // N�pisy u komponent
  JLabel canc2 = new JLabel();

  public OknoVoleb(Frame vlastnik, Object[] vol) // Konstruktor
	{
		super(vlastnik, "Volby", true);
                volby = vol;
		setDefaultCloseOperation(JDialog.HIDE_ON_CLOSE);
                    try {
                     jbInit();
                }
                catch(Exception e) {
                    e.printStackTrace();
                }
	}

  public OknoVoleb() {      // Konstruktor (nepou��v� se, poz�statek prvn�ho n�padu)
    try {
      jbInit();
    }
    catch(Exception e) {
      e.printStackTrace();
    }
  }

  public boolean getStiskOK(){return stiskOK;}  // Jak bylo okno uzav�eno

  private void jbInit() throws Exception {    // Inicializace komponent
    this.getContentPane().setLayout(null);
    this.setResizable(false);
    //this.setModal(true);
    this.setSize(new Dimension(448, 300));
    ok.setText("OK");
    storno.setText("Storno");
    ok.setBounds(new Rectangle(30, 230, 110, 30));
    ok.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        ok_actionPerformed(e);
      }
    });
    storno.setBounds(new Rectangle(145, 230, 110, 30));
    storno.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        storno_actionPerformed(e);
      }
    });
    obrazek1.setBounds(new Rectangle(30, 35, 353, 30));
    obrazek1.setText((String)volby[0]);
    obrazek2.setText((String)volby[1]);
    obrazek2.setBounds(new Rectangle(30, 70, 353, 30));

    rozmery = new Object[HORNI_MEZ-DOLNI_MEZ+1];  // P��prava pole zobrazovan�ch hodnot pro kombo
    for(int i = DOLNI_MEZ; i <= HORNI_MEZ; i++){
      rozmery[i-DOLNI_MEZ] = new Integer(i);
    }

    kombo = new JComboBox(rozmery);
    kombo.setBounds(new Rectangle(30, 160, 87, 26));
    kombo.setEditable(true);

    kombo.setSelectedIndex(rozmery.length-1);

    canc1.setText("Soubor obsahuj�c� znak pro 1., resp. 2. hr��e:");
    canc1.setBounds(new Rectangle(30, 10, 354, 18));
    canc2.setText("Po�et �ad a sloupc� hrac�ch pol�:");
    canc2.setBounds(new Rectangle(32, 126, 306, 17));
    this.getContentPane().add(ok, null);
    this.getContentPane().add(storno, null);
    this.getContentPane().add(obrazek1, null);
    this.getContentPane().add(obrazek2, null);
    this.getContentPane().add(kombo, null);
    this.getContentPane().add(canc1, null);
    this.getContentPane().add(canc2, null);
  }

  void storno_actionPerformed(ActionEvent e) {  // Odezva na stisk Storno
    stiskOK = false;                            // Nastav p��znak
    obrazek1.setText((String)volby[0]);         // Obnov p�vodn� volby
    obrazek2.setText((String)volby[1]);
    kombo.setSelectedIndex(rozmery.length-1);
    this.setVisible(false);      // Skryj okno
  }

  void ok_actionPerformed(ActionEvent e) {// Odezva na stisk OK
    volby[0] = obrazek1.getText();    // P�enes volby
    volby[1] = obrazek2.getText();
    int n = Integer.parseInt(kombo.getSelectedItem().toString());
    n = Math.max(MINIMUM, n);// Po�et pol��ek mus� b�t v rozmez� MINIMUM, HORNI_MEZ
    volby[2] = new Integer(Math.min(HORNI_MEZ, n));
    stiskOK = true;// Nastav p��znak
    this.setVisible(false);// Skryj okno
  }
}