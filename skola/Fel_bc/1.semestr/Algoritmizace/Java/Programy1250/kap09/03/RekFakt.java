/* Soubor Kap09\03\RekFakt.java
 * rekurzivn� definice funkce faktori�l
 */

public class RekFakt {
  static int f(int n)
  {
    if(n == 0) return 1;
    else return n*f(n-1);  // Metoda f() vol� sama sebe
  }

  public RekFakt() {
  }
  public static void main(String[] args) {
    System.out.print("Zadej cislo: ");
    int n = MojeIO.inInt();
    System.out.print("Faktorial "+n+" je "+ f(n));
  }
}