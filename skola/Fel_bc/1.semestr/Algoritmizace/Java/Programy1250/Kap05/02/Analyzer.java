/* Soubor Kap05\02\Analyzer.java 
   Vznikl �pravou souboru Kap04\03\Analyzer.java

   Jedin� rozd�l: pou�it� p��kazu import umo�n� nepsat
   cel� jm�no java.util.ArrayList, ale jen ArrayList

   Program, kter� vyp�e v�echna r�zn� slova, kter� se vyskytuj�
   v textov�m souboru, a po�et jejich opakov�n�.
   �te standardn� vstup. Vyu��v� MojeIO, tzn. MojeIO.class mus� b�t v n�kter�m z adres���
   ur�en�ch syst�movou prom�nnou CLASSPATH.

   pou�it�: java Analyzer < jm�noSouboru

   Neum� ��st p��mo data z konzole.
*/

import java.util.ArrayList;

// T��da, kter� slou�� k ukl�d�n� dvojic slovo - po�et v�skyt�
class Dvojice 
{
  private int pocet;	// po�et v�skyt�
  private String slovo; // �et�zec (slovo)
  public void pridej(){pocet++;}	// Zv�t�i po�et v�skyt�
  public Dvojice(String s, int i){pocet = i; slovo = s;}	// Konstruktory
  public Dvojice(String s){pocet = 1; slovo = s;}
  public boolean equals(Object x){return slovo.equals(((Dvojice)x).slovo);}// P�edefinovan� metoda pro provn�v�n� dvojic - porovn�v� podle �et�zce
  public String toString(){return slovo + "(" + pocet + ")";}	// P�edefinovan� metoda pro konverzi na �et�zec
}

// Hlavn� t��da programu
public class Analyzer
{
  static String oddelovace = " .,;!?";		// seznam znak�, kter� mohou odd�lovat slova v textu

  
  int preskocOddelovace(String rad, int od) // rad: analyzovan� ��dek, od: index, od kter�ho se m� za��t
  {					    // Vrac�: index, kde za��n� n�sleduj�c� slovo, nebo -1, jsme-li na konci ��dku	
    if(od >= rad.length()) return -1;       // Jsme-li na konci ��dku, vra� -1
    while(oddelovace.indexOf(rad.charAt(od))>=0)
    {
     if(++od == rad.length()) return -1;
    }
    return od;
  }

  void pridej(String slovo, ArrayList sezn) // P�id�n� nov�ho slova do seznamu 
  {						     // nebo zv��en� po�tu v�skyt�	
	Dvojice d = new Dvojice(slovo);			// Vytvo� dvojici
	int i = sezn.indexOf(d);			// Zkus jin naj�t v seznamu
	if(i<0) 					// Kdy� tam nen�
	{sezn.add(d);}					// tak ji p�idej
	else						// jinak
	{
	 d = (Dvojice)sezn.get(i);			// si ji vyndej a 
	 d.pridej();					// zv�t�i po�et v�skyt�
	}
  }
  void analyzuj(String radek, ArrayList sezn) // radek: analyzovan� ��dek, sezn: Kontejner, do kterho se maj�� slova ukl�dat
  {
    if(radek.equals(""))return;		// Pr�zdn� ��dek n�s nezaj�m�
    int i = 0;				// Index znaku v ��dku
    StringBuffer slovo = null;		// Sem ulo��me z�skan� slovo
    i = preskocOddelovace(radek, i);	// Najdi za��tek dal��ho slova (-1 == konec ��dku)
    while(i >= 0)
    {
      slovo = new StringBuffer("");
      // P�enes n�sleduj�c� slovo do instance "slovo"
      while((i < radek.length()) && (oddelovace.indexOf(radek.charAt(i))==-1)) // Dokud to nen� odd�lova�
      {
        slovo.append(radek.charAt(i++));
      }
      // Ulo� slovo do slovn�ku
      String s = new String(slovo);
      pridej(s, sezn);
      i = preskocOddelovace(radek, i);
    }
  }

  void beh() throws Exception		// Vlastn� b�h programu
  {
  	ArrayList slovnik = new ArrayList();
        String radek = inStr();	// P�e�ti 1. ��dek

        while(radek != null)		// Dokud se �ten� da��
        {
          radek = radek.trim();		// Odstra� okrajov� mezery
          analyzuj(radek, slovnik);
	  radek = MojeIO.inStr();	// �ti dal�� ��dek
        }
        System.out.println(slovnik);    // Vypi� v�sledek
  }

  public static void main(String[] s) throws Exception
  { // Vytvo�� instanci t��dy Analyzer a spust� pro ni metodu beh()
      Analyzer a = new Analyzer();
      a.beh();
  }
}