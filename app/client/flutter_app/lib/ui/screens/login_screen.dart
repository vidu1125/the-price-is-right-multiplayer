import 'package:flutter/material.dart';
import '../theme/login_theme.dart';
import '../../services/service_locator.dart';

class LoginScreen extends StatefulWidget {
  const LoginScreen({super.key});

  @override
  State<LoginScreen> createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {
  final _formKey = GlobalKey<FormState>();
  final _emailController = TextEditingController();
  final _passwordController = TextEditingController();
  bool _isLoading = false;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      body: Stack(
        children: [
          // Background Image
          Positioned.fill(
            child: Image.asset("assets/images/lobby-bg.png", fit: BoxFit.cover),
          ),
          Positioned.fill(child: Container(color: Colors.black.withOpacity(0.5))),

          Center(
          child: SingleChildScrollView(
            padding: const EdgeInsets.all(24),
            child: Container(
              constraints: const BoxConstraints(maxWidth: 480),
              padding: const EdgeInsets.all(40),
              decoration: LoginTheme.cardDecoration,
              child: Form(
                key: _formKey,
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text("Sign In", style: LoginTheme.headingStyle),
                    const SizedBox(height: 8),
                    const Text("Welcome back! Please enter your details.", 
                      style: TextStyle(color: LoginTheme.textSecondary)),
                    const SizedBox(height: 32),
                    
                    // Email Field
                    const Text("Email", style: LoginTheme.labelStyle),
                    const SizedBox(height: 8),
                    TextFormField(
                      controller: _emailController,
                      decoration: LoginTheme.inputDecoration("you@example.com"),
                      validator: (val) => (val == null || !val.contains('@')) ? "Invalid email" : null,
                    ),
                    const SizedBox(height: 24),
                    
                    // Password Field
                    const Text("Password", style: LoginTheme.labelStyle),
                    const SizedBox(height: 8),
                    TextFormField(
                      controller: _passwordController,
                      obscureText: true,
                      decoration: LoginTheme.inputDecoration("Your password"),
                      validator: (val) => (val == null || val.length < 6) ? "Min 6 characters" : null,
                    ),
                    
                    const SizedBox(height: 32),
                    
                    // CTA Button
                    GestureDetector(
                      onTap: _isLoading ? null : () async {
                        if (_formKey.currentState!.validate()) {
                          setState(() => _isLoading = true);
                          
                          print("🔴 Sending Login Request...");
                          final result = await ServiceLocator.authService.login(
                            _emailController.text, 
                            _passwordController.text
                          );
                          print("🔴 Login Result: $result");

                          setState(() => _isLoading = false);

                          if (result["success"] == true) {
                            if (mounted) {
                              Navigator.pushReplacementNamed(context, '/lobby');
                            }
                          } else {
                            if (mounted) {
                              ScaffoldMessenger.of(context).showSnackBar(
                                SnackBar(
                                  content: Text(result["error"] ?? "Login failed"),
                                  backgroundColor: Colors.red,
                                ),
                              );
                            }
                          }
                        }
                      },
                      child: Container(
                        width: double.infinity,
                        height: 55,
                        decoration: BoxDecoration(
                          borderRadius: BorderRadius.circular(14),
                          gradient: LoginTheme.ctaGradient,
                        ),
                        alignment: Alignment.center,
                        child: _isLoading 
                          ? const CircularProgressIndicator(color: Colors.white)
                          : const Text("Sign in", 
                              style: TextStyle(color: Colors.white, fontSize: 18, fontWeight: FontWeight.bold)),
                      ),
                    ),
                    
                    const SizedBox(height: 24),
                    
                    // Footer
                    Row(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        const Text("No account yet? ", style: TextStyle(color: LoginTheme.textSecondary)),
                        TextButton(
                          onPressed: () => Navigator.pushNamed(context, '/register'),
                          child: const Text("Register here", 
                            style: TextStyle(color: LoginTheme.darkBlue, fontWeight: FontWeight.bold)),
                        ),
                      ],
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ],
      ),
    );
  }
}