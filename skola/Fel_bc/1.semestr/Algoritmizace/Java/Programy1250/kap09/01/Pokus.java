/* Soubor Kap09\01\Pokus.java
 * p�edv�d�, �e se parametry primitivn�ch typ� p�ed�vaj� hodnotou,
 * ale parametry objektov�ch typ� odkazem
 * (p�esn�ji, hodnotou se p�ed�v� odkaz na objekt nebo pole)
 *
 * T��da Pokus obsahuje dv� metody f(). Ob� po��taj� faktori�l 
 * a ob� p�itom m�n� hodnotu form�ln�ho parametru.
 *
 * Jedna m� parametr typu int (zde nedojde ke zm�n� skute�n�ho parametru),
 * druh� m� parametr typu MujInt, co� je objektov� typ zapouzd�uj�c�
 * int. Jsou pro n�j definov�ny metody, kter� umo��uj� operace n�soben�, -- atd.
 * P�i vol�n� t�to metody se skute�n� parametr zm�n�.
 */

class MujInt {                  // T��da MujInt je objektov� typ zapouzd�uj�c� int a operace s n�m
  private int i;                // Ulo�en� cel� ��slo
  MujInt(int x){ i = x; }       // Konstruktor
  int getInt(){ return i; }     // Vr�t� ulo�en� cel� ��slo
  int minusMinus(){ return --i; }               // --
  boolean vetsiNez(int x){ return i > x; }      // Porovn�n� s cel�m ��slem
  void setInt(int x){i = x;}                    // Zm�na ulo�en� hodnoty
  void kratRovnaSe(MujInt n){ i *= n.getInt(); }// Oper�tor *=
}

public class Pokus {		
  static MujInt f(MujInt n)     // V�po�et faktori�lu pro typ MujInt
  {                             // T�m�� stejn� jako v�po�et pro int v n�sleduj�c�
    MujInt s;                   // metod� f(int)
    for(s = new MujInt(1); n.vetsiNez(0); n.minusMinus())
      s.kratRovnaSe(n);
    return s;
  }

  static int f(int n)           // V�po�et faktori�lu pro typ int
  {
    int s;
    for(s = 1; n > 0; n--) s *= n;
    return s;
  }

  public static void main(String[] args) {       // pokusy s p�ed�v�n�m parametr�
    int x = 5, y = f(x);
    System.out.println("faktorial " + x + " je " + y);
    MujInt xx = new MujInt(5), yy = f(xx);
    System.out.println("faktorial " + xx.getInt() + " je " + yy.getInt());
  }
}