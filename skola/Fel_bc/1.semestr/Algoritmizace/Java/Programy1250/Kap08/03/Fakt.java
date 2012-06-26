/* Soubor Kap07\03\Fakt.java
   V�po�et faktori�lu, tentokr�t pomoci for
   samostatn� funkce pro v�po�et faktori�lu
   Vyu��v� slo�eb t��dy MojeIO, tak�e je t�eba m�t v adres��i,
   do kter�ho ukazuje CLASSPATH, soubor MojeIO.class

   Neobsahuje ��dn� kontroly spr�vnosti vstupu
*/

public class Fakt {
  public static int faktorial(int n)
  {
	int s = 1;		// Prom�nn� s bude obsahovat v�sledek
        for(int i = n; i > 0; i--) s *= i;
 	return s;
  }
  public static void main(String[] arg)
  {
    	System.out.print("Zadej cele cislo: ");
	int n = MojeIO.inInt();	// Vstup hodnoty
	System.out.println("Jeho faktorial je " + faktorial(n));
  }
}