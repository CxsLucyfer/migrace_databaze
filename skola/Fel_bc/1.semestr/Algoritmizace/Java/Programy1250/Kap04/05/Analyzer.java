/*
   Soubor Kap4\05\Analyzer.java
   St�le po��t�n� slov v textov�m souboru,
   ov�em tentokr�t s ponoc� vlastn� implementace 
   jednosm�rn� z�et�zen�ho seznamu.
   Pou�it� stejn� jako v p�edchoz�ch p��padech.
   Vhodn� pro JDK 1.2 a vy���
*/

/** Prvek seznamu */
class Prvek
{
  String slovo = null;       // Slovo ulo�en� v prvku
  int pocet = 0;             // po�et v�skyt�
  Prvek dalsi = null;        // Odkaz na dal�� prvek seznamu
  Prvek(){}                  // Konstruktory
  Prvek(String s) {slovo = s; pocet = 1;}
  int obsahuje(String s) {return s.equals(slovo)? pocet : 0;}
  void setDalsi(Prvek q){dalsi = q;}
  Prvek getDalsi(){return dalsi;}
  String getSlovo(){return slovo;}
  void zvetsiPocet(){pocet++;}
  int getPocet(){return pocet;}
}

/** Vlastn� implementace seznamu*/
class Seznam
{
  Prvek hlava = null;
  void pridej(String slovo)
  {
    if(hlava == null)
    {
      hlava = new Prvek(slovo);
    }
    else
    {
       Prvek p = hlava, q = null;
       while(p != null)
       {
          if(p.obsahuje(slovo) > 0)
          {
            p.zvetsiPocet();
            return;
          }
          else
          {
            q = p;
            p = p.getDalsi();
          } // Konec if
       }    // konec while
       q.setDalsi(new Prvek(slovo));
    }
  }

  void vypis()
  {
     Prvek p = hlava;
     while(p != null)
     {
       System.out.println(p.getSlovo()+ " " + p.getPocet());
       p = p.getDalsi();
     }
  }
  
  void vyprazdni()
  {
	hlava = null;
  }

}

public class Analyzer
{
  static String oddelovace = " .,;!?";

  int preskocOddelovace(String rad, int od) // rad: analyzovan� ��dek, od: index, od kter�ho se m� za��t
  {					    // Vrac�: index, kde za��n� n�sleduj�c� slovo, nebo -1, jsme-li na konci ��dku	
    if(od >= rad.length()) return -1;       // Jsme-li na konci ��dku, vra� -1
    while(oddelovace.indexOf(rad.charAt(od))>=0)
    {
     if(++od == rad.length()) return -1;
    }
    return od;
  }



  void analyzuj(String radek, Seznam sezn) // radek: analyzovan� ��dek, sezn: Kontejner, do kterho se maj�� slova ukl�dat
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
      sezn.pridej(s);
      i = preskocOddelovace(radek, i);
    }
  }


  void beh() throws Exception		// Vlastn� b�h programu
  {
    	Seznam slovnik = new Seznam();
        String radek = MojeIO.inStr();	// P�e�ti 1. ��dek

        while(radek != null)		// Dokud se �ten� da��
        {
          radek = radek.trim();		// Odstra� okrajov� mezery
          analyzuj(radek, slovnik);
	  radek = MojeIO.inStr();	// �ti dal�� ��dek
        }
        slovnik.vypis();    // Vypi� v�sledek

  }

  public static void main(String[] s) throws Exception
  { // Vytvo�� instanci t��dy Analyzer a spust� pro ni metodu beh()
      Analyzer a = new Analyzer();
      a.beh();
  }
}