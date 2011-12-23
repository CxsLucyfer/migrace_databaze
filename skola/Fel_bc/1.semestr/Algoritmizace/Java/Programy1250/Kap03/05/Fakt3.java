/* Soubor Kap03\05\Fakt3.java
   v�po�et faktori�lu, t�et� pokus 
   samostatn� funkce pro v�po�et faktori�lu.
   Verze pro JDK 5 -- pou��v� metodu printf().
   Ve star��ch verz�ch jazyka je t�eba odstranit p��kaz 
   System.out.printf("Jeho...);
   nahradit ho p��kazem
   // System.out.println("Jeho..);
   kter� je za n�m uzav�en v koment��i
*/
public class Fakt3 {
  public static int faktorial(int n)
  {
		int s = 1;	// Prom�nn� s bude obsahovat v�sledek
    	if(n < 0)
    	{
			System.out.println("Neni definovan");
    		System.exit(0);
    	} else {		// Cyklus, ve kter�m se faktori�l vypo�te
			while(n > 1)
			{
				s *= n--;
			}
		}
 		return s;
  }
  public static void main(String[] arg)
  {
   	System.out.print("Zadej cele cislo: ");
	int m = MojeIO.inInt();	// Vstup hodnoty
	System.out.printf("Jeho faktorial je %d\n", faktorial(m));
        // Ve star��ch verz�ch JDK tento p��kaz 
        // nahrad�me n�sleduj�c�m:
	// System.out.println("Jeho faktorial je " + faktorial(m));
  }
}
