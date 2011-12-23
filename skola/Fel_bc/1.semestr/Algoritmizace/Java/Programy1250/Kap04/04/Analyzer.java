/* Soubor Kap04\04\Analyzer.java

   Pouze pro JDK 5

   Program, kter� vyp�e v�echna r�zn� slova, kter� se vyskytuj�
   v textov�m souboru, a po�et jejich opakov�n�.
   �te standardn� vstup. Vyu��v� parametrizovan� typ ArrayList<>.

   pou�it�: java Analyzer < jm�noSouboru

   Neum� ��st p��mo data z konzoly.
*/

// T��da, kter� slou�� k ukl�d�n� dvojic slovo - po�et v�skyt�
class Dvojice 
{
  private int pocet;    // po�et v�skyt�
  private String slovo; // �et�zec (slovo)
  public void pridej(){pocet++;}	// Zv�t�i po�et v�skyt�
  public Dvojice(String s, int i){pocet = i; slovo = s;}        // Konstruktory
  public Dvojice(String s){pocet = 1; slovo = s;}
  public boolean equals(Object x){return slovo.equals(((Dvojice)x).slovo);}// P�edefinovan� metoda pro provn�v�n� dvojic - porovn�v� podle �et�zce
  public String toString(){return slovo + "(" + pocet + ")";}   // P�edefinovan� metoda pro konverzi na �et�zec
}

// Hlavn� t��da programu
public class Analyzer
{
  static String oddelovace = " .,;!?";         // seznam znak�, kter� mohou odd�lovat slova v textu

  
  String odstra�Odd�lova�e(String slo)         // slo: slovo, ze kter�ho chceme odstranit odd�lova�e
  {                                            // Vrac�: Slovo bez odd�lova��	
    StringBuffer s = new StringBuffer();
    int i = 0;
    while(i < slo.length() && oddelovace.indexOf(slo.charAt(i)) == -1)
    {
	s.append(slo.charAt(i));
        i++;
    }
    return s.toString();
  }

  // P�id�n� nov�ho slova do seznamu 
  void pridej(String slovo, java.util.ArrayList<Dvojice> sezn) 
  {						     // nebo zv��en� po�tu v�skyt�	
	Dvojice d = new Dvojice(slovo);			// Vytvo� dvojici
	int i = sezn.indexOf(d);			// Zkus ji naj�t v seznamu
	if(i<0) 					// Kdy� tam nen�
	{sezn.add(d);}					// tak ji p�idej
	else						// jinak
	{
	 d = sezn.get(i);			        // si ji vyndej a 
	 d.pridej();					// zv�t�i po�et v�skyt�
	}
  }
 

  void beh()		// Vlastn� b�h programu
  {
        java.util.Scanner skan=new java.util.Scanner(System.in);
	String slovo;
	java.util.ArrayList<Dvojice> sezn = new java.util.ArrayList<Dvojice>();

	while(skan.hasNext())		// Dokud je co ��st
        {
          slovo = skan.next();		// Odstra� okrajov� mezery
          pridej(odstra�Odd�lova�e(slovo), sezn);
        }
        System.out.println(sezn);        // Vypi� v�sledek
  }

  public static void main(String[] s)
  { // Vytvo�� instanci t��dy Analyzer a spust� pro ni metodu beh()
      Analyzer a = new Analyzer();
      a.beh();
  }
}