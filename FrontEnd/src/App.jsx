import React, {
  useEffect,
  useState
} from "react";

import Header from "./components/Header";
import StatusCard from "./components/StatusCard";
import Alerts from "./components/Alerts";
import HistoryTable from "./components/HistoryTable";
import Footer from "./components/Footer";

import {
  getStatus,
  getHistorico
} from "./services/api";

function App() {

  const [status, setStatus] = useState({
    racao: 0,
    wifi: false
  });

  const [historico, setHistorico] =
    useState([]);

  useEffect(() => {

    async function carregarDados() {

      try {

        const dados =
          await getStatus();

        setStatus(dados);

const historicoBanco =
  await getHistorico();

if (Array.isArray(historicoBanco)) {
  setHistorico(historicoBanco);
} else {
  console.log("Resposta do histórico:", historicoBanco);
  setHistorico([]);
}

      } catch (erro) {

        console.log(erro);

      }
    }

    carregarDados();

    const intervalo =
      setInterval(
        carregarDados,
        5000
      );

    return () =>
      clearInterval(intervalo);

  }, []);

  return (

    <div className="container">

      <Header />

      <div className="grid">

        <StatusCard
          titulo="Ração"
          valor={`${status.racao}%`}
          emoji="🍖"
        />

        <StatusCard
          titulo="Wi-Fi"
          valor={
            status.wifi
              ? "Conectado"
              : "Offline"
          }
          emoji="📶"
        />

      </div>

      <Alerts racao={status.racao} />

      <HistoryTable
        historico={historico}
      />

      <Footer />

    </div>

  );
}

export default App;