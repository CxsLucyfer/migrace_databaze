/* Soubor Kap15\01\Aplet1.java
 * Jednoduch� aplet, kter� podle stisknut�ho p�ep�na�e
 * zobrazuje bu� cit�t nebo jeho autora.
 * Oba texty �te z parametr� apletu v HTML str�nce.
 * Pokud se parametry nepoda�� na��st, zobraz� implicitn� text
 * "Chaos vl�dne i bez ministr�." a jako autora uvede
 * "Bobbyho p�esv�d�en�".
 * Cit�t v parametrech apletu poch�z� novoro�n�ho p��pitku Milo�e Formana 
 * (�T, silvestr 2000), 
 * cit�t pou�it� v apletu jako implicitn� je z knihy
 * Artura Blocha "Murphyho z�kon", vydan� nakladatelstv�m Svoboda - Libertas
 * v Praze r. 1993.
 */

import java.awt.*;
import java.awt.event.*;
import javax.swing.*;

public class Aplet1 extends JApplet {
  String citat = "Chaos vl�dne i bez ministr�.";
  String autor = "Bobbyho p�esv�d�en�";
  JLabel napis = new JLabel();
  JPanel pomocny = new JPanel();
  ButtonGroup bg = new ButtonGroup();
  JRadioButton zobrazCitat = new JRadioButton();
  JRadioButton zobrazAutora = new JRadioButton();

  // P�e�te hodnotu parametru, a pokud neusp�je, vr�t� implicitn� hodnotu
  public String getParameter(String key, String def) {
      return (getParameter(key) != null ? getParameter(key) : def);
  }

  public Aplet1() {  // Konstruktor
  }

  public void init() { // Inicializace
    try {
      String _citat = getParameter("VYROK", citat);
      String _autor =  getParameter("AUTOR", autor);
      citat = _citat;
      autor = _autor;
    }
    catch(Exception e) {}
    try {
      jbInit();
    }
    catch(Exception e) {
      e.printStackTrace();
    }
  }

  private void jbInit() throws Exception {   // Inicializace komponent
    this.setSize(new Dimension(400,300));
    JPanel panel = (JPanel)getContentPane();

    napis.setText(citat);
    napis.setFont(new java.awt.Font("Serif", Font.ITALIC | Font.BOLD, 32));
    napis.setForeground(Color.red);
    napis.setHorizontalAlignment(SwingConstants.CENTER);
    panel.add(napis, BorderLayout.CENTER);

    panel.add(pomocny, BorderLayout.SOUTH); 

    bg.add(zobrazCitat);		// P�idej p�ep�na�e do skupiny
    bg.add(zobrazAutora);
    pomocny.add(zobrazCitat);		// a vlo� je na spodn� panel
    pomocny.add(zobrazAutora);
    zobrazCitat.setText("Cit�t");
    zobrazCitat.setSelected(true);
    zobrazAutora.setText("Autor");
    ActionListener prijemce = new ActionListener() {// Instance p��jemce ud�losti
        public void actionPerformed(ActionEvent e) {
            stiskPrepinace(e);
        };
    };
    zobrazCitat.addActionListener(prijemce);
    zobrazAutora.addActionListener(prijemce);
  }

  public String getAppletInfo() {// Informace o apletu
    return "M�j prvn� aplet";
  }

  protected void stiskPrepinace(ActionEvent e) { // Handler pro stisknut�
    if(e.getSource().equals(zobrazCitat))     // p�ep�na�e cit�t - autor
      napis.setText(citat);
    else
      napis.setText(autor);
  }
}