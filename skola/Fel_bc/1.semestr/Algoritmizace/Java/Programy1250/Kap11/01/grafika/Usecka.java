/* Soubor Kap011\01\grafika\Usecka.java
 * Ve srovn�n� s Kap09\05 je zde nav�c metoda zkontroluj()
 * D�d�n�m od t��dy GO z�skala tak� implementaci
 * rozhran� kontrola.Kontrola
 */

package grafika;

public class Usecka extends GO {
  private Bod a, b;
  public Bod getA(){ return a;}
  public void setA(Bod _a){ a = _a;}
  public Bod getB(){ return b;}
  public void setB(Bod _b){ b = _b;}
  public boolean zkontroluj()
  {
    return super.zkontroluj() && a.zkontroluj() && b.zkontroluj();
  }

  public void nakresli(){
    System.out.println("Kreslim Usecku, barva " +
                    getBarva()+", krajn� body:");
    a.nakresli();
    b.nakresli();
  }

  public Usecka(int _x1, int _y1, int _x2, int _y2, int _barva) {
    super(_barva);
    a = new Bod(_x1, _y1, _barva);
    b = new Bod(_x2, _y2, _barva);
  }
}