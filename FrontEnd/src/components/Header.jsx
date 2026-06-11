import React from "react";

export default function Header() {
  return (
    <header className="header">
      <div className="titulo-container">
        <h1>Patinhas Smart</h1>

        <img
          src="/logo.png"
          alt="Logo Patinhas Smart"
          className="logo"
        />
      </div>

      <p>Monitoramento Inteligente de Ração</p>
    </header>
  );
}