/* Soubor Kap03\05\Fakt2.java
   v�po�et faktori�lu, druh� pokus 
*/

public class Fakt2 {
  public static void main(String[] arg)
  {
    	System.out.print("Zadej cele cislo: ");
	int n = MojeIO.inInt();	// Vstup hodnoty
	int s = 1;		// Prom�nn� s bude obsahovat v�sledek
    	if(n < 0)
    	{
		System.out.println("Neni definovan");
    		System.exit(0);
    	} else {
		while(n > 1)	// Cyklus, ve kter�m se faktori�l vypo�te
		{
			s *= n--;
		}
	}
	System.out.println("Jeho faktorial je " + s);
  }
}