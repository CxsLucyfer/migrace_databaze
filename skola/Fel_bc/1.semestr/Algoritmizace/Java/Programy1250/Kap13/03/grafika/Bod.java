/* Soubor Kap13\03\grafika\Bod.java
 * D�d�n�m od t��dy GO z�skala implementaci
 * rozhran� kontrola.Kontrola, Cloneable a Serializable
 *
 * Metodu clone() nen� t�eba p�edefinovat, nebo� pln� vyhovuje
 * zd�d�n� metoda.
 *
 * Ve srovn�n� s p�edchoz� verz� (Kap11\01) je zde nav�c nov�
 * metoda toString(), kter� vr�t� text "Bod " a �daje o sou�adnic�ch a barv�.
 * Je tak� zm�n�na metoda nakresli(), kter� vyu�ije metodu toString().
 */
package grafika;

public class Bod extends GO {
  private int x, y;

  public int getX(){return x;}
  public void setX(int _x){ x = _x;}
  public int getY(){return y;}
  public void setY(int _y){ y = _y;}
  public boolean zkontroluj() {
	return super.zkontroluj() && (x > 0) && (y > 0);
  }
  public String toString(){return "Bod (" + x + ", " + y + "), barva " + getBarva(); }
  public void nakresli(){
    System.out.println(this);
  }
  public Bod(int _x, int _y, int _barva) {
    super(_barva);
    setX(_x);
    setY(_y);
  }
//  public Object clone(){return super.clone();}
}