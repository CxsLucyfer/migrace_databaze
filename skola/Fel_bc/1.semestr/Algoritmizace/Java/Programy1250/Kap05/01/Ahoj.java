/* Soubor Kap05\01\ahoj\Ahoj.java
 * Analogie prvn�ho programu s pou�it�m bal�ku
 * package mus� b�t prvn� p��kaz, p�ed n�m sm�j� b�t
 * jen pr�zdn� ��dky nebo koment��e
 */

package pozdrav;

public class Ahoj {
  void text(){
	System.out.println("To uz tady bylo ... a uz je to tu zas.");
  }
  public static void main(String[] s)
  {
	Ahoj a = new Ahoj();
	a.text();
  }
}