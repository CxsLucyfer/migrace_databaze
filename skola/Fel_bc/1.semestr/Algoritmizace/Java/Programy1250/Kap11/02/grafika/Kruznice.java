/* Soubor Kap11\01\grafika\Kruznice.java
 * Ve srovn�n� s Kap09\05 je zde nav�c metoda zkontroluj()
 * D�d�n�m od t��dy GO z�skala tak� implementaci
 * rozhran� kontrola.Kontrola a rozhrani Cloneable

 * metoda clone()  nejprve pomoc� metody p�edka vytvo��
 * bin�rn� kopii instance, pak vytvo�� kopii st�edu, proto�e
 * "klonovan�" kru�nice mus� m�t svuj st�ed, kter� p�jde zm�nit,
 * ani� bychom zm�nili st�ed p�vodn� kru�nice.
 *
 * Ve srovn�n� s p�edchoz� verz� (Kap11\01) je zde nav�c nov� 
 * metoda toString(), kter� vr�t� text "Kruzice " a �daje o barv�, polom�ru a st�edu.
 * Je tak� zm�n�na metoda nakresli(), kter� vyu�ije metodu toString().
 */

package grafika;

public class Kruznice extends GO {
  Bod stred;
  int r;

  public int getR(){return r;}
  public void setR(int _r){r = _r;}
  public Bod getStred(){return stred;}
  public void setStred(Bod _stred){stred = _stred;}
  public boolean zkontroluj()
  {
    return super.zkontroluj() && stred.zkontroluj() && (r > 0);
  }
  public void nakresli(){
    System.out.println(this);
  }

  public String toString() {
    return  "Kruznice, barva " + getBarva()+ ", polomer " + getR() +
            ", stred: " + stred;
  }

  public Object clone () throws CloneNotSupportedException {
    Kruznice k = (Kruznice)super.clone();
    k.stred = (Bod)stred.clone();
    return k;
  }
  public Kruznice(int _x, int _y, int _r, int _barva) {
    super(_barva);
    stred = new Bod(_x, _y, _barva);
    r = _r;
  }

}