package alg12;

// Fronta cel�ch ��sel implementovan� spojov�m seznamem

class PrvekFronty {
  PrvekFronty dalsi;
  int hodn;
}

public class FrontaCisel {
  private PrvekFronty celo;
  private PrvekFronty volny;

  public FrontaCisel() {
    celo = new PrvekFronty();
    volny = celo;
  }

  public void vloz(int x) {
    volny.hodn = x;
    volny.dalsi = new PrvekFronty();
    volny = volny.dalsi;
  }

  public int odeber() {
    if (jePrazdna())
      throw new RuntimeException("odebr�n� z pr�zdn� fronty");
    int vysl = celo.hodn;
    celo = celo.dalsi;
    return vysl;
  }

  public boolean jePrazdna() {
    return celo == volny;
  }

  public int znakNaCele()
  {return celo.hodn;}

}