import React, { useEffect, useState } from "react";

import Header from "./components/Header";
import StatusCard from "./components/StatusCard";
import Alerts from "./components/Alerts";
import HistoryTable from "./components/HistoryTable";
import Footer from "./components/Footer";

import { getStatus } from "./services/api";

function App() {
  const [status, setStatus] = useState({
    racao: 0,
    wifi: false
  });

  const [historico, setHistorico] = useState([
    { hora: "08:00", racao: 90 },
    { hora: "10:00", racao: 75 },
    { hora: "12:00", racao: 60 }
  ]);

  useEffect(() => {
    async function carregarDados() {
      try {
        const dados = await getStatus();
        setStatus(dados);
      } catch (erro) {
        console.log(erro);
      }
    }

    carregarDados();

    const intervalo = setInterval(
      carregarDados,
      5000
    );

    return () => clearInterval(intervalo);
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

      <HistoryTable historico={historico} />

      <Footer />
    </div>
  );
}

export default App;