/* Soubor Kap10\01\Fakt10.java

 * Op�t faktori�l (to tu u� dlouho nebylo), 
 * ale tentokr�t nejprve testuje, zda le�� parametr
 * v rozmez� 0 - 20, jinak vyvol� v�jimku.
 * (Pro n < 0 nen� faktori�l definov�n, pro n > 20 
 * se v�sledek nevejde do rozsahu typu long.) 
 * Metoda je vol�na se �patn�m parametrem, a proto skon��
 * v�jimkou.
 */

public class Fakt10 {
  public static long fakt(int n){
    if((n < 0) || (n > 20)) throw new ArithmeticException();
    int s = 1;
    while(n > 1) s *= n--;
    return s;
  }
  public static void main(String[] args) {
    System.out.print(fakt(-5));
  }
}