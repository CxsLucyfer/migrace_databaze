/* Soubor Kap11\02\grafika\Usecka.java
 * Ve srovn�n� s Kap09\05 je zde nav�c metoda zkontroluj()
 * D�d�n�m od t��dy GO z�skala tak� implementaci
 * rozhran� kontrola.Kontrola a Cloneable
 *
 * metoda clone()  nejprve pomoc� metody p�edka vytvo��
 * bin�rn� kopii instance, pak vytvo�� kopie krajn�ch bod�, proto�e
 * "klonovan�" �se�ka mus� m�t sv� krajn� body, kter� p�jde zm�nit,
 * ani� bychom zm�nili krajn� body p�vodn� �se�ky.
 *
 * Ve srovn�n� s p�edchoz� verz� (Kap11\01) je zde nav�c nov�
 * metoda toString(), kter� vr�t� text "Kruzice " a �daje o polom�ru a st�edu.
 * Je tak� zm�n�na metoda nakresli(), kter� vyu�ije metodu toString().

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

  public Object clone() throws CloneNotSupportedException {
    Usecka u = (Usecka)super.clone();
    u.a = (Bod)a.clone();
    u.b = (Bod)b.clone();
    return u;
  }
  public void nakresli(){
    System.out.println(this);
  }

  public String toString() {
    return  "Usecka, barva " + getBarva()+ ", pocatek: " + a +
            ", konec: " + b;
  }


  public Usecka(int _x1, int _y1, int _x2, int _y2, int _barva) {
    super(_barva);
    a = new Bod(_x1, _y1, _barva);
    b = new Bod(_x2, _y2, _barva);
  }
}