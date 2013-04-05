/* Soubor Kap11\01\grafika\Bod.java
 * Ve srovn�n� s Kap09\05 je zde nav�c metoda zkontroluj()
 * D�d�n�m od t��dy GO z�skala tak� implementaci
 * rozhran� kontrola.Kontrola
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
  public void nakresli(){
    System.out.println("Kreslim Bod (" + x + ", "+ y + "), barva " + getBarva());
  }
  public Bod(int _x, int _y, int _barva) {
    super(_barva);
    setX(_x);
    setY(_y);
  }
}