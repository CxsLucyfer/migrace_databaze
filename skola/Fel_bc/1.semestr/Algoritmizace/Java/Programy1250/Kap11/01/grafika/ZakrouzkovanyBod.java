/* Soubor Kap11\01\grafika\ZakrouzkovanyBod.java
 * Metoda zkontroluj() je stejna jako v p�edkovi, nen� tedy t�eba ji m�nit
 * D�d�n�m od t��dy GO z�skala tak� implementaci
 * rozhran� kontrola.Kontrola
 */
package grafika;

public class ZakrouzkovanyBod extends Bod {

  public ZakrouzkovanyBod(int _x, int _y, int _barva) {
   super(_x, _y, _barva);
  }
  private void nakresliKrouzek()
  {
    System.out.println("Kreslim krouzek");
  }
  public void nakresli(){
    super.nakresli();
    nakresliKrouzek();
  }
}