/* Soubor Kap14\07\OknoVoleb.java
 * Okno voleb pro program Pi�korky 2.1
 * Definuje konstanty ud�vaj�c� rozmez� po�tu tla��tek
 * Umo��uje zadat nejm�n� MINIMUM tla��tek, nejv��e HORNI_MEZ
 * Kombinovan� seznam nab�z� hodnoty od DOLNI_MEZ po HORNI_MEZ
 * Okno se po uzav�en� nezru��, pouze skryje
 * Vylep�eno o filtr v okn� pro volbu souboru
 */

package vokno;
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;

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
  JButton volic1 = new JButton();
  JButton volic2 = new JButton();

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
    volic1.setText("...");
    volic1.setBounds(new Rectangle(390, 36, 32, 30));
    volic2.setBounds(new Rectangle(391, 71, 32, 30));
    volic2.setText("...");
    volic1.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        volba_Akce(e);
      }
    });
    volic2.addActionListener(new java.awt.event.ActionListener() {
      public void actionPerformed(ActionEvent e) {
        volba_Akce(e);
      }
    });
    this.getContentPane().add(ok, null);
    this.getContentPane().add(storno, null);
    this.getContentPane().add(obrazek1, null);
    this.getContentPane().add(obrazek2, null);
    this.getContentPane().add(kombo, null);
    this.getContentPane().add(canc1, null);
    this.getContentPane().add(canc2, null);
    this.getContentPane().add(volic1, null);
    this.getContentPane().add(volic2, null);
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

  // Vnit�n� t��da (jej� deklarace je vno�ena do t��dy OknoVoleb)
  // ** T��da filtru nemus� b�t vnit�n� **
  //
  // T��da filtru pro v�b�r soubor� zobrazovan�ch v dialogov�m okn� JFileChooser
  // Mus� b�t potomkem t��dy javax.swing.filechooser.FileFilter
  // a mus� implementovat ve�ejn� p��stupnou metodu
  // boolean accept(File f),
  // kter� rozhoduje, zda se soubor zobraz�
  // Metoda
  // public String getDescription()
  // vr�t� popis, kter� se zobraz� v ��dce pod jm�nem souboru (Files of type)
  class Filtr extends javax.swing.filechooser.FileFilter
  {
    private String pripony = ".jpg .png .jpeg .gif"; // Akceptovan� p��pony
    public boolean accept(File soubor)
    {
      String pripona = getExtension(soubor);
      if(pripony.indexOf(pripona) != -1)return true;
      else return false;
    }
    private String getExtension(File soubor)  // Z�sk� p��ponu souboru
    {
      if(soubor != null)
      {
        String jmeno = soubor.getName();      // Z�sk� cel� jm�no
        int i = jmeno.lastIndexOf('.');       // Te�kou za��n� p��pona
        if (i > 0 && i < jmeno.length()-1)    // Vyjmi p��ponu a p�eve� ji na mal� p�smena
         return jmeno.substring(i+1).toLowerCase();// zkus ji naj�t v �et�zci pripony
      }
      return "";                // Kdy� tam nen�, vra� pr�zdn� �et�zec
    }
    public String getDescription() // Vr�t� popis
    {
      return "Grafick� soubory pro Javu";
    }
  }

  void volba_Akce(ActionEvent e){
   JTextField pole = e.getSource().equals(volic1)? obrazek1:obrazek2;

    File soubor = new File(".");

    JFileChooser vyberSoubor = new JFileChooser();
    vyberSoubor.setCurrentDirectory(soubor);
    vyberSoubor.setDialogTitle("Nov� symbol hr��e");
    vyberSoubor.setFileFilter(new Filtr());       // Nastav filtr
    int vysledek = vyberSoubor.showOpenDialog(this);
    if(vysledek == JFileChooser.APPROVE_OPTION)
    {
      pole.setText(vyberSoubor.getSelectedFile().getPath());
    }
  }
}