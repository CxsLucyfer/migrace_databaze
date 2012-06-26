// Soubor Kap08\06\Assert.java
// OD JDK 1.4 V݊E
// Ukazuje pou�it� aserce
//
// V JDK 1.4 nutno p�elo�it p��kazem
// javac -source 1.4 Assert.java 
// 
// V JDK 5 lze p�ep�na� -source vynechat
//
// P�i spu�t�n� je t�eba pou��t p��kaz
// java -ea Assert
//
// P�ep�na� -ea je t�eba pou��t i v JDK 5

public class Assert {
  public static int faktorial(int n)
  {
    if (Pomoc.LADIM) 
      assert n >= 0: "Zaporny parametr faktorialu: " + n;
    int s = 1;      
    for(int i = n; i > 0; i--) 
                  s *= i;
    return s;
  }


  public static void main(String[] s) {
    System.out.println("Volam faktorial");
    int y = -1;
    int x = faktorial(-1);
    System.out.println("Faktorial " + y + " je " + x);
  }
}