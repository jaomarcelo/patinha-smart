import React, { useEffect, useState } from "react";

import Header from "./components/Header";
import StatusCard from "./components/StatusCard";
import Alerts from "./components/Alerts";
import HistoryTable from "./components/HistoryTable";
import Footer from "./components/Footer";

import { getStatus, getHistorico } from "./services/api";

function App() {
  const [status, setStatus] = useState({
    racao: 0,
    wifi: false
  });

  const [historico, setHistorico] = useState([]);

  useEffect(() => {
    async function carregarDados() {
      try {
        const dados = await getStatus();
        
        // Comunicação limpa: o backend agora sempre manda "racao" e "wifi"
        setStatus({
          racao: dados.racao !== undefined ? dados.racao : 0,
          wifi: dados.wifi !== undefined ? dados.wifi : false
        });

        const historicoBanco = await getHistorico();

        if (Array.isArray(historicoBanco)) {
          // Inverte o array para mostrar os envios mais recentes no topo da tabela
          setHistorico(historicoBanco.reverse());
        } else {
          setHistorico([]);
        }

      } catch (erro) {
        console.error("Erro na comunicação com a API IoT:", erro);
      }
    }

    // Chama a primeira vez imediatamente
    carregarDados();

    // Mantém a sincronização em tempo real a cada 5 segundos
    const intervalo = setInterval(carregarDados, 5000);

    return () => clearInterval(intervalo);
  }, []);

  return (
    <div className="container">
      <Header />

      <div className="grid">
        <StatusCard
          titulo="Nível da Ração"
          valor={`${Number(status.racao).toFixed(1)} g`}
          emoji="🍖"
        />

        <StatusCard
          titulo="Conexão Wi-Fi"
          valor={status.wifi ? "Online" : "Offline"}
          emoji={status.wifi ? "🟢" : "🔴"} 
        />
      </div>

      <Alerts racao={status.racao} />

      <HistoryTable historico={historico} />

      <Footer />
    </div>
  );
}

export default App;
