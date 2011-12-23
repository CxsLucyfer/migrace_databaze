/* Soubor Kap11\02\grafika\GO.java
 * Podobn� jako v Kap09\05, t��da GO je ale
 * abstraktn� a implementuje rozhran� kontrola.Kontrola a Cloneable
 * Tato rozhran� pak d�d� i odvozen� t��dy.
 *
 * Metoda clone() prost� zavol� zd�d�nou od t��dy Object,
 * nebo� nepot�ebujeme nic v�c ne� bin�rn� kopii instance.
 * Museli jsme ji v�ak p�edefinovat, nebo� ji chceme zve�ejnit.
 */
package grafika;

import kontrola.*;

public abstract class GO implements Kontrola, Cloneable {
  private int barva;

  public int getBarva() { return barva;}
  public void setBarva(int _barva){ barva = _barva;}
  public GO() {}
  public GO(int _barva){setBarva(_barva);}
  public boolean zkontroluj() {return barva > 0;}
  public abstract void nakresli();
  public Object clone() throws CloneNotSupportedException {return super.clone();}
}