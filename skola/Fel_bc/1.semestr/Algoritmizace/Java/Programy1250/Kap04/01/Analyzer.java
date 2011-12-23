/* Soubor Kap04\01\Analyzer.java
   Program, kter� vyp�e v�echna r�zn� slova, kter� se vyskytuj�
   v textov�m souboru.
   �te standardn� vstup. Vyu��v� MojeIO, tzn. MojeIO.class 
   mus� b�t v n�kter�m z adres���
   ur�en�ch syst�movou prom�nnou CLASSPATH.

   pou�it�: java Analyzer < jm�noSouboru

   Pou�iteln� ve v�ech verz�ch JDK, v JDK 5 ov�em dostaneme 
   varov�n�, �e pou��v� typov� nezabezpe�en� operace. 
   Toto varov�n� zat�m ignorujeme.

   Neum� ��st p��mo data z konzoly.
*/

// Hlavn� a jedin� t��da programu
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

  void analyzuj(String radek, java.util.ArrayList sezn) // radek: analyzovan� ��dek, sezn: Kontejner, do kterho se maj�� slova ukl�dat
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
      if(sezn.indexOf(s)==-1)sezn.add(new String(s));
      i = preskocOddelovace(radek, i);
    }
  }

  void beh() throws java.io.IOException		// Vlastn� b�h programu
  {
  	java.util.ArrayList slovnik = new java.util.ArrayList();
        String radek = MojeIO.inStr();	// P�e�ti 1. ��dek

        while(radek != null)		// Dokud se �ten� da��
        {
          radek = radek.trim();		// Odstra� okrajov� mezery
          analyzuj(radek, slovnik);
	  radek = MojeIO.inStr();	// �ti dal�� ��dek
        }
        System.out.println(slovnik);    // Vypi� v�sledek
  }

  public static void main(String[] s) throws java.io.IOException
  { // Vytvo�� instanci t��dy Analyzer a spust� pro ni metodu beh()
      Analyzer a = new Analyzer();
      a.beh();
  }
}