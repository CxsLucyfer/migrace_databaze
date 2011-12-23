/* Soubor Kap06\02\Fakt.java
   v�po�et faktori�lu - uk�zka p�ete�en�
   samostatn� funkce pro v�po�et faktori�lu
   v cyklu �te cel� ��sla z konzoly a vypisuje jejich faktori�ly.
   Skon��, zad�me-li z�pornou hodnotu.
*/

public class Fakt {
  public static int faktorial(int n)
  {
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
 	return s;
  }

  public static long faktorial(long n)
  {
	long s = 1;		// Prom�nn� s bude obsahovat v�sledek
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
 	return s;
  }
  public static void main(String[] arg)
  {
    	
	int i;
	i = 'A';
	System.out.print("Zadej cele cislo: ");
	int n = MojeIO.inInt();	// Vstup hodnoty
	while(n >= 0) {
	  System.out.println("Jeho faktorial je " + faktorial((long)n));
	  System.out.print("Zadej cele cislo: ");
	  n = MojeIO.inInt();	// Vstup hodnoty
	}
  }
}