#!/usr/bin/env python3
"""
Build script interativo para projeto Android com libs nativas (.so)
Este script fornece uma interface CLI conversacional para builds Gradle
"""

import os
import sys
import subprocess
import platform

class AndroidBuilder:
    def __init__(self):
        self.project_root = os.path.dirname(os.path.abspath(__file__))
        self.gradle_command = self.get_gradle_command()
        
    def get_gradle_command(self):
        """Detecta o comando Gradle apropriado para o SO"""
        if platform.system() == "Windows":
            return "gradle.bat"
        else:
            return "gradle"
    
    def run_gradle_command(self, command):
        """Executa comando Gradle e retorna resultado"""
        try:
            # No Windows, usar shell=True para comandos .bat
            if platform.system() == "Windows":
                cmd = f"{self.gradle_command} {command}"
                print(f"\n🔨 Executando: {cmd}")
                print("=" * 50)
                
                result = subprocess.run(cmd, cwd=self.project_root, 
                                      shell=True, text=True)
            else:
                cmd = [self.gradle_command] + command.split()
                print(f"\n🔨 Executando: {' '.join(cmd)}")
                print("=" * 50)
                
                result = subprocess.run(cmd, cwd=self.project_root, 
                                      capture_output=False, text=True)
            
            if result.returncode == 0:
                print("\n✅ Build concluído com sucesso!")
            else:
                print(f"\n❌ Build falhou com código: {result.returncode}")
                
            return result.returncode == 0
            
        except Exception as e:
            print(f"❌ Erro ao executar comando: {e}")
            print(f"💡 Dica: Verifique se o Gradle está instalado e no PATH do sistema")
            return False
    
    def show_menu(self):
        """Exibe menu principal"""
        print("\n" + "="*60)
        print("🏗️  ANDROID BUILDER - West Gunfighter Hooks")
        print("="*60)
        print("1. 🛠️  Build Native Libraries (.so)")
        print("0. 🚪 Sair")
        print("="*60)
    
    def build_debug(self):
        """Build em modo debug"""
        print("\n🔧 Iniciando build DEBUG...")
        return self.run_gradle_command("assembleDebug")
    
    def build_release(self):
        """Build em modo release"""
        print("\n🚀 Iniciando build RELEASE...")
        return self.run_gradle_command("assembleRelease")
    
    def clean_project(self):
        """Limpa o projeto"""
        print("\n🧹 Limpando projeto...")
        return self.run_gradle_command("clean")
    
    def rebuild_project(self):
        """Clean + Build debug"""
        print("\n🔄 Iniciando REBUILD (Clean + Debug)...")
        if self.clean_project():
            return self.build_debug()
        return False
    
    def assemble_debug_apk(self):
        """Gera APK debug"""
        print("\n📦 Gerando APK DEBUG...")
        return self.run_gradle_command("assembleDebug")
    
    def assemble_release_apk(self):
        """Gera APK release"""
        print("\n📦 Gerando APK RELEASE...")
        return self.run_gradle_command("assembleRelease")
    
    def build_native_libs(self):
        """Build específico das bibliotecas nativas (.so)"""
        print("\n🛠️  Compilando bibliotecas nativas (.so)...")
        success = self.run_gradle_command("externalNativeBuildDebug")
        
        if success:
            self.show_so_path()
        
        return success
    
    def show_so_path(self):
        """Mostra o caminho do arquivo .so gerado"""
        so_paths = [
            "app/build/intermediates/ndk/debug/lib/armeabi-v7a/libWestGunfighterHooksVdev.so",
            "app/build/intermediates/cmake/debug/obj/armeabi-v7a/libWestGunfighterHooksVdev.so",
            "app/build/intermediates/ndkBuild/debug/obj/local/armeabi-v7a/libWestGunfighterHooksVdev.so"
        ]
        
        print("\n📁 LOCALIZAÇÃO DO ARQUIVO .SO:")
        print("-" * 50)
        
        found = False
        for relative_path in so_paths:
            full_path = os.path.join(self.project_root, relative_path)
            if os.path.exists(full_path):
                print(f"✅ Encontrado: {full_path}")
                
                # Mostra informações do arquivo
                stat = os.stat(full_path)
                size_kb = stat.st_size / 1024
                print(f"📏 Tamanho: {size_kb:.1f} KB")
                print(f"🕒 Modificado: {os.path.getmtime(full_path)}")
                found = True
                break
        
        if not found:
            print("❌ Arquivo .so não encontrado nos caminhos esperados")
            print("🔍 Procurando em outros locais...")
            
            # Busca recursiva por arquivos .so
            for root, dirs, files in os.walk(os.path.join(self.project_root, "app", "build")):
                for file in files:
                    if file.endswith(".so") and "WestGunfighterHooksVdev" in file:
                        full_path = os.path.join(root, file)
                        print(f"✅ Encontrado: {full_path}")
                        found = True
                        break
                if found:
                    break
            
            if not found:
                print("❌ Nenhum arquivo .so encontrado")
    
    def show_project_info(self):
        """Mostra informações do projeto"""
        print("\n🔍 INFORMAÇÕES DO PROJETO")
        print("-" * 40)
        print(f"📁 Diretório: {self.project_root}")
        print(f"⚙️  Comando Gradle: {self.gradle_command}")
        print(f"🖥️  SO: {platform.system()}")
        
        # Verifica se gradle está disponível
        try:
            result = subprocess.run([self.gradle_command, "--version"], 
                                  capture_output=True, text=True)
            if result.returncode == 0:
                print("✅ Gradle encontrado no sistema")
            else:
                print("❌ Gradle NÃO encontrado no sistema")
        except FileNotFoundError:
            print("❌ Gradle NÃO encontrado no PATH do sistema")
            
        # Lista arquivos de build importantes
        important_files = ["build.gradle", "app/build.gradle", "settings.gradle", 
                          "app/src/main/jni/Android.mk"]
        
        print("\n📋 Arquivos de configuração:")
        for file in important_files:
            path = os.path.join(self.project_root, file)
            status = "✅" if os.path.exists(path) else "❌"
            print(f"  {status} {file}")
    
    def show_help(self):
        """Mostra ajuda"""
        print("\n❓ AJUDA")
        print("-" * 40)
        print("Este script automatiza builds do projeto Android com NDK.")
        print("\n📖 Comandos disponíveis:")
        print("• Debug: Compila versão de desenvolvimento")
        print("• Release: Compila versão otimizada para produção")
        print("• Clean: Remove arquivos de build anteriores")
        print("• Rebuild: Limpa e reconstrói o projeto")
        print("• Native Libs: Compila apenas as bibliotecas .so")
        print("\n🎯 Biblioteca gerada: WestGunfighterHooksVdev.so")
        print("📁 Localização: app/build/intermediates/ndk/debug/lib/armeabi-v7a/")
    
    def run(self):
        """Loop principal da aplicação"""
        print("🎮 Bem-vindo ao Android Builder!")
        
        while True:
            self.show_menu()
            
            try:
                choice = input("\n👉 Escolha uma opção: ").strip()
                
                if choice == "0":
                    print("\n👋 Saindo... Até logo!")
                    break
                elif choice == "1":
                    self.build_native_libs()
                else:
                    print("\n❌ Opção inválida! Escolha 1 para build ou 0 para sair.")
                
                # Pausa para ver resultado
                if choice != "0":
                    input("\n⏸️  Pressione ENTER para continuar...")
                    
            except KeyboardInterrupt:
                print("\n\n👋 Interrompido pelo usuário. Saindo...")
                break
            except Exception as e:
                print(f"\n❌ Erro inesperado: {e}")

def main():
    """Função principal"""
    builder = AndroidBuilder()
    builder.run()

if __name__ == "__main__":
    main()