/* Soubor Kap10\01\Fakt10a.java
 * Je�t� jednou v�po�et faktori�lu, 
 * tentokr�t v�jimku zachyt�me a o�et��me v metod� main()
 *
 * Vy�aduje pou�it� t��dy MojeIO pro vstup cel�ch ��sel
 * (najdete ji v Kap03\io) 
 */

public class Fakt10a {
  public static long fakt(int n){
    if((n < 0) || (n > 20)) throw new ArithmeticException();
    int s = 1;
    while(n > 1) s *= n--;
    return s;
  }
  public static void main(String[] args) {
    try {
     System.out.print("Zadej cele cislo: ");
     int i = MojeIO.inInt();
     System.out.println("Jeho faktorial je "+ fakt(i));
     System.out.println("Vypocet probehl bez problemu");
    }
    catch(ArithmeticException e){
      System.out.println("Cislo musi lezet v rozmezi od 0 do 20");
      e.printStackTrace();
    }
    System.out.println("Ferda Mravenec, prace vseho druhu");  }
}