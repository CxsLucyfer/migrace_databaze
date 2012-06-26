/* Soubor Kap11\01\grafika\GO.java
 * Podobn� jako v Kap09\05, t��da GO je ale
 * abstraktn� a implementuje rozhran� kontrola.Kontrola .
 * Toto rozhran� pak d�d� i odvozen� t��dy.
 */
package grafika;

import kontrola.*;

public abstract class GO implements Kontrola {
  private int barva;

  public int getBarva() { return barva;}
  public void setBarva(int _barva){ barva = _barva;}
  public GO() {}
  public GO(int _barva){setBarva(_barva);}
  public boolean zkontroluj() {return barva > 0;}
  public abstract void nakresli();
}