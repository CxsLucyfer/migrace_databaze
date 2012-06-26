/* Soubor Kap13\03\grafika\GO.java
 * Podobn� jako v Kap09\05, t��da GO je ale
 * abstraktn� a implementuje rozhran� kontrola.Kontrola, Cloneable a Serializable
 * Tato rozhran� pak d�d� i odvozen� t��dy.
 * Serializaci nen� t�eba implementovat
 */
package grafika;

import kontrola.*;
import java.io.*;

public abstract class GO implements Kontrola, Cloneable, Serializable {
  private int barva;

  public int getBarva() { return barva;}
  public void setBarva(int _barva){ barva = _barva;}
  public GO() {}
  public GO(int _barva){setBarva(_barva);}
  public boolean zkontroluj() {return barva > 0;}
  public abstract void nakresli();
  public Object clone() throws CloneNotSupportedException {return super.clone();}
}